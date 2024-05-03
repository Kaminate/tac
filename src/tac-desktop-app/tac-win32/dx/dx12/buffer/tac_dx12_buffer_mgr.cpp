#include "tac_dx12_buffer_mgr.h" // self-inc
#include "tac-win32/dx/dx12/tac_dx12_enum_helper.h"
#include "tac-win32/dx/dx12/descriptor/tac_dx12_descriptor_heap.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h"
#include "tac-win32/dx/dx12/context/tac_dx12_context.h"
#include "tac-win32/dx/dx12/context/tac_dx12_context_manager.h"

namespace Tac::Render
{

  static D3D12_RESOURCE_FLAGS GetResourceFlags( Binding binding )
  {
    D3D12_RESOURCE_FLAGS ResourceFlags{ D3D12_RESOURCE_FLAG_NONE };

    if( Binding{} == ( binding & Binding::ShaderResource ) )
      ResourceFlags &= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

    if( Binding{} != ( binding & Binding::UnorderedAccess ) )
      ResourceFlags &= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    return ResourceFlags;
  }

  DX12BufferMgr::DescriptorBindings DX12BufferMgr::CreateBindings( ID3D12Resource* resource,
                                                                   Binding binding )
  {
    Optional< DX12DescriptorHeapAllocation > srv;
    Optional< DX12DescriptorHeapAllocation > uav;

    if( Binding{} != ( binding & Binding::ShaderResource ) )
    {
      const DX12DescriptorHeapAllocation allocation{ mCpuDescriptorHeapCBV_SRV_UAV->Allocate() };
      const D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor{ allocation.GetCPUHandle() };
      mDevice->CreateShaderResourceView( resource, nullptr, DestDescriptor );
      srv = allocation;
    }

    if( Binding{} != ( binding & Binding::ShaderResource ) )
    {
      const DX12DescriptorHeapAllocation allocation{ mCpuDescriptorHeapCBV_SRV_UAV->Allocate() };
      const D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor{ allocation.GetCPUHandle() };
      mDevice->CreateUnorderedAccessView( resource, nullptr, nullptr, DestDescriptor );
      srv = allocation;
    }

    return DescriptorBindings
    {
      .mSRV { srv },
      .mUAV { uav },
    };
  }

  void DX12BufferMgr::Init( Params params )
  {
    mDevice = params.mDevice;
    mCpuDescriptorHeapCBV_SRV_UAV = params.mCpuDescriptorHeapCBV_SRV_UAV;
    mContextManager = params.mContextManager;
    TAC_ASSERT( mDevice && mCpuDescriptorHeapCBV_SRV_UAV && mContextManager );
  }

  void DX12BufferMgr::CreateBuffer( BufferHandle h,
                                    CreateBufferParams params,
                                    Errors& errors)
  {
    const int byteCount { params.mByteCount };
    const StackFrame sf { params.mStackFrame };

    if( params.mUsage == Usage::Dynamic )
    {
      DX12Buffer* buffer { &mBuffers[  h.GetIndex()  ] };
      *buffer = DX12Buffer{ .mCreateParams { params }, };
      if( params.mOptionalName )
      {
        buffer->mCreateName = params.mOptionalName;
        buffer->mCreateParams.mOptionalName = buffer->mCreateName;
      }

      return;
    }


    D3D12_HEAP_TYPE heapType{ D3D12_HEAP_TYPE_DEFAULT };
    if( params.mUsage == Usage::Staging )
      heapType = D3D12_HEAP_TYPE_UPLOAD;

    const D3D12_HEAP_PROPERTIES HeapProps
    {
      .Type                 { heapType },
      .CPUPageProperty      { D3D12_CPU_PAGE_PROPERTY_UNKNOWN },
      .MemoryPoolPreference { D3D12_MEMORY_POOL_UNKNOWN },
      .CreationNodeMask     { 1 },
      .VisibleNodeMask      { 1 },
    };

    const DXGI_SAMPLE_DESC SampleDesc
    {
      .Count   { 1 },
      .Quality { 0 },
    };

    const D3D12_RESOURCE_FLAGS ResourceFlags{ GetResourceFlags( params.mBinding ) };

    const D3D12_RESOURCE_DESC ResourceDesc
    {
      .Dimension        { D3D12_RESOURCE_DIMENSION_BUFFER },
      .Alignment        { 0 },
      .Width            { ( UINT64 )byteCount },
      .Height           { 1 },
      .DepthOrArraySize { 1 },
      .MipLevels        { 1 },
      .Format           { DXGI_FORMAT_UNKNOWN },
      .SampleDesc       { SampleDesc },
      .Layout           { D3D12_TEXTURE_LAYOUT_ROW_MAJOR },
      .Flags            { ResourceFlags },
    };

    const D3D12_RESOURCE_STATES DefaultUsage{
      [ & ]()
      {
        if( heapType == D3D12_HEAP_TYPE_UPLOAD )
          return D3D12_RESOURCE_STATE_GENERIC_READ;
        if( params.mBytes )
          return D3D12_RESOURCE_STATE_COPY_DEST;
        return D3D12_RESOURCE_STATE_COMMON;
      }( ) };

    ID3D12Device* device{ mDevice };

    PCom< ID3D12Resource > buffer;
    TAC_DX12_CALL( device->CreateCommittedResource(
      &HeapProps,
      D3D12_HEAP_FLAG_NONE,
      &ResourceDesc,
      DefaultUsage,
      nullptr,
      buffer.iid(),
      buffer.ppv() ) );

    ID3D12Resource* resource{ buffer.Get() };
    DX12SetName( resource, sf );

    void* mappedCPUAddr{};
    if( heapType == D3D12_HEAP_TYPE_UPLOAD )
        buffer->Map( 0, nullptr, &mappedCPUAddr );

    if( params.mBytes )
    {
      if( mappedCPUAddr )
      {
        MemCpy( mappedCPUAddr, params.mBytes, params.mByteCount );
      }
      else
      {
        DX12Context* context { mContextManager->GetContext(errors ) };
        ID3D12GraphicsCommandList* commandList { context->GetCommandList() };
        DX12UploadAllocator GPUUploadAllocator{ context->mGPUUploadAllocator };

        DX12UploadAllocator::DynAlloc allocation{
          GPUUploadAllocator.Allocate( byteCount, errors ) };
        MemCpy( allocation.mCPUAddr, params.mBytes, params.mByteCount );

        ID3D12Resource* dstResource { buffer.Get() };
        const UINT64 dstResourceOffset{};
        commandList->CopyBufferRegion( dstResource,
                                       dstResourceOffset,
                                       allocation.mResource,
                                       allocation.mResourceOffest,
                                       params.mByteCount );

        TAC_CALL( context->Execute( errors ) );
        context->Retire();
      }
    }

    DescriptorBindings descriptorBindings;
    if( resource )
      descriptorBindings = CreateBindings( resource, params.mBinding );

    const int i { h.GetIndex() };
    mBuffers[ i ] = DX12Buffer
    {
      .mResource      { buffer },
      .mDesc          { ResourceDesc },
      .mState         { DefaultUsage },
      .mSRV           { descriptorBindings.mSRV },
      .mUAV           { descriptorBindings.mUAV },
      .mMappedCPUAddr { mappedCPUAddr },
    };
  }


  void DX12BufferMgr::UpdateBuffer( BufferHandle h,
                                    UpdateBufferParams params,
                                    DX12Context* context,
                                    Errors& errors )
  {

    DX12Buffer& buffer { mBuffers[ h.GetIndex() ] };
    ID3D12Resource* resource { buffer.mResource.Get() };
    if( !buffer.mMappedCPUAddr )
    {
      const int byteCount{ buffer.mCreateParams.mByteCount };

        ID3D12GraphicsCommandList* commandList { context->GetCommandList() };
        DX12UploadAllocator GPUUploadAllocator{ context->mGPUUploadAllocator };

        DX12UploadAllocator::DynAlloc allocation{
          GPUUploadAllocator.Allocate( byteCount, errors ) };

        buffer.mMappedCPUAddr = allocation.mCPUAddr;
    }

    char* dst{ ( char* )buffer.mMappedCPUAddr + params.mDstByteOffset };
    MemCpy( dst,
            params.mSrcBytes,
            params.mSrcByteCount );

    TAC_ASSERT( buffer.mMappedCPUAddr );
    char* dstBytes { ( char* )buffer.mMappedCPUAddr + params.mDstByteOffset };
    MemCpy( dstBytes, params.mSrcBytes, params.mSrcByteCount );
  }

  void DX12BufferMgr::DestroyBuffer( BufferHandle h )
  {
    if( h.IsValid() )
      mBuffers[ h.GetIndex() ] = {};
  }
} // namespace Tac::Render
