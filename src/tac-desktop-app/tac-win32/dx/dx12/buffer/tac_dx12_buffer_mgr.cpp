#include "tac_dx12_buffer_mgr.h" // self-inc

#include "tac-win32/dx/dx12/tac_dx12_enum_helper.h"
#include "tac-win32/dx/dx12/descriptor/tac_dx12_descriptor_heap.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h"
#include "tac-win32/dx/dx12/context/tac_dx12_context.h"
#include "tac-win32/dx/dx12/context/tac_dx12_context_manager.h"
#include "tac-win32/dx/dx12/tac_dx12_transition_helper.h"
#include "tac-win32/dx/dxgi/tac_dxgi.h"

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

  static DXGI_FORMAT GetSRVFormat( CreateBufferParams params )
  {
    if( params.mGpuBufferMode == GpuBufferMode::kStructured )
      return GetDXGIFormatTexture( params.mGpuBufferFmt );

    if( params.mGpuBufferMode == GpuBufferMode::kByteAddress )
      return DXGI_FORMAT_R32_TYPELESS;

    return DXGI_FORMAT_UNKNOWN;
  }


  DX12BufferMgr::DescriptorBindings DX12BufferMgr::CreateBindings( ID3D12Resource* resource,
                                                                   CreateBufferParams params )
  {

    const Binding binding{ params.mBinding };

    Optional< DX12Descriptor > srv;
    Optional< DX12Descriptor > uav;

    if( Binding{} != ( binding & Binding::ShaderResource ) )
    {
      const DX12Descriptor allocation{ mCpuDescriptorHeapCBV_SRV_UAV->Allocate() };
      const D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor{ allocation.GetCPUHandle() };

      TAC_ASSERT( params.mGpuBufferMode != GpuBufferMode::kUndefined );

      const D3D12_BUFFER_SRV_FLAGS Flags{
        params.mGpuBufferMode == GpuBufferMode::kByteAddress
        ? D3D12_BUFFER_SRV_FLAG_RAW
        : D3D12_BUFFER_SRV_FLAG_NONE };

      const UINT NumElements{
        params.mGpuBufferMode == GpuBufferMode::kByteAddress
        ? ( UINT )params.mByteCount / 4 // due to DXGI_FORMAT_R32_TYPELESS
        : ( UINT )params.mByteCount / ( UINT )params.mStride
      };

      const UINT StructureByteStride{
        params.mGpuBufferMode == GpuBufferMode::kStructured
        ? ( UINT )params.mStride
        : ( UINT )0 };

      const D3D12_BUFFER_SRV Buffer
      {
        .FirstElement        { 0 },
        .NumElements         { NumElements },
        .StructureByteStride { StructureByteStride },
        .Flags               { Flags },
      };

      const DXGI_FORMAT Format{ GetSRVFormat( params ) };

      const D3D12_SHADER_RESOURCE_VIEW_DESC Desc
      {
        .Format                  { Format },
        .ViewDimension           { D3D12_SRV_DIMENSION_BUFFER },
        .Shader4ComponentMapping { D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING },
        .Buffer                  { Buffer },
      };

      mDevice->CreateShaderResourceView( resource, &Desc, DestDescriptor );
      srv = allocation;
    }

    if( Binding{} != ( binding & Binding::UnorderedAccess ) )
    {
      const DX12Descriptor allocation{ mCpuDescriptorHeapCBV_SRV_UAV->Allocate() };
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

    D3D12_RESOURCE_STATES resourceStates{
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
      resourceStates,
      nullptr,
      buffer.iid(),
      buffer.ppv() ) );

    ID3D12Resource* resource{ buffer.Get() };

    const DX12Name name
    {
      .mName          { params.mOptionalName },
      .mStackFrame    { params.mStackFrame },
      .mResourceType  { "Buffer" },
      .mResourceIndex { h.GetIndex() },
    };
    DX12SetName( resource, name );

    void* mappedCPUAddr{};
    if( heapType == D3D12_HEAP_TYPE_UPLOAD )
        resource->Map( 0, nullptr, &mappedCPUAddr );

    if( params.mBytes )
    {
      if( mappedCPUAddr )
      {
        MemCpy( mappedCPUAddr, params.mBytes, params.mByteCount );
      }
      else
      {
        DX12Context::Scope contextScope{ mContextManager->GetContext( errors ) };
        DX12Context* context{ ( DX12Context* )contextScope.GetContext() };
        ID3D12GraphicsCommandList* commandList { context->GetCommandList() };
        DX12UploadAllocator* GPUUploadAllocator{ &context->mGPUUploadAllocator };

        DX12UploadAllocator::DynAlloc allocation{
          GPUUploadAllocator->Allocate( byteCount, errors ) };
        MemCpy( allocation.mCPUAddr, params.mBytes, params.mByteCount );

        const UINT64 dstResourceOffset{};
        commandList->CopyBufferRegion( resource,
                                       dstResourceOffset,
                                       allocation.mResource,
                                       allocation.mResourceOffest,
                                       params.mByteCount );

        // do we context->SetSynchronous() ?
        TAC_CALL( context->Execute( errors ) );
      }
    }

    DescriptorBindings descriptorBindings;
    if( resource )
      descriptorBindings = CreateBindings( resource, params );


    // Transition to the intended usage so that the state will be correct for a descriptor table
    {
      D3D12_RESOURCE_STATES usageFromBinding{ D3D12_RESOURCE_STATE_COMMON };

      if( Binding{} != ( params.mBinding & Binding::ShaderResource ) )
        usageFromBinding |= D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;

      if( Binding{} != ( params.mBinding & Binding::RenderTarget ) )
      {
        usageFromBinding |= D3D12_RESOURCE_STATE_RENDER_TARGET;
        // D3D12_RESOURCE_STATE_PRESENT ?
      }

      if( Binding{} != ( params.mBinding & Binding::DepthStencil ) )
      {
        usageFromBinding |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
        // D3D12_RESOURCE_STATE_DEPTH_READ ?
      }

      if( Binding{} != ( params.mBinding & Binding::UnorderedAccess ) )
        usageFromBinding |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

      if( Binding{} != ( params.mBinding & Binding::ConstantBuffer ) )
        usageFromBinding |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

      if( Binding{} != ( params.mBinding & Binding::VertexBuffer ) )
        usageFromBinding |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
      
      if( Binding{} != ( params.mBinding & Binding::IndexBuffer ) )
        usageFromBinding |= D3D12_RESOURCE_STATE_INDEX_BUFFER;


      DX12Context::Scope contextScope{ mContextManager->GetContext( errors ) };
      DX12Context* context{ ( DX12Context* )contextScope.GetContext() };
      ID3D12GraphicsCommandList* commandList { context->GetCommandList() };

      const DX12TransitionHelper::Params transitionParams
      {
        .mResource    { resource },
        .mStateBefore { &resourceStates },
        .mStateAfter  { usageFromBinding },
      };
      DX12TransitionHelper transitionHelper;
      transitionHelper.Append( transitionParams );
      transitionHelper.ResourceBarrier( commandList );
      // do we context->SetSynchronous() ?
      TAC_CALL( context->Execute( errors ) );
    }





    const D3D12_GPU_VIRTUAL_ADDRESS gpuVritualAddress { buffer->GetGPUVirtualAddress() };

    const int i { h.GetIndex() };
    mBuffers[ i ] = DX12Buffer
    {
      .mResource       { buffer },
      .mDesc           { ResourceDesc },
      .mState          { resourceStates },
      .mGPUVirtualAddr { gpuVritualAddress },
      .mSRV            { descriptorBindings.mSRV },
      .mUAV            { descriptorBindings.mUAV },
      .mMappedCPUAddr  { mappedCPUAddr },
      .mCreateParams   { params },
    };
  }

  DX12Buffer* DX12BufferMgr::FindBuffer( BufferHandle h )
  {
    return h.IsValid() ? &mBuffers[ h.GetIndex() ] : nullptr;
  }

  void DX12BufferMgr::UpdateBuffer( BufferHandle h,
                                    UpdateBufferParams params,
                                    DX12Context* context,
                                    Errors& errors )
  {

    DX12Buffer& buffer{ mBuffers[ h.GetIndex() ] };
    ID3D12Resource* resource{ buffer.mResource.Get() };
    if( !buffer.mMappedCPUAddr )
    {
      const int byteCount{ buffer.mCreateParams.mByteCount };

      ID3D12GraphicsCommandList* commandList{ context->GetCommandList() };
      DX12UploadAllocator* GPUUploadAllocator{ &context->mGPUUploadAllocator };

      DX12UploadAllocator::DynAlloc allocation{
        GPUUploadAllocator->Allocate( byteCount, errors ) };

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
