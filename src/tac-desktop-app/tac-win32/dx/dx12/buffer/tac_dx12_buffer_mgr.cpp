#include "tac_dx12_buffer_mgr.h" // self-inc
#include "tac-win32/dx/dx12/tac_dx12_enum_helper.h"
#include "tac-win32/dx/dx12/descriptor/tac_dx12_descriptor_heap.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h"

namespace Tac::Render
{

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
    TAC_ASSERT( mDevice && mCpuDescriptorHeapCBV_SRV_UAV );
  }

  void DX12BufferMgr::CreateBuffer( BufferHandle h, CreateBufferParams params, Errors& errors)
  {
    const int byteCount { params.mByteCount };
    const StackFrame sf { params.mStackFrame };

    Optional< D3D12_HEAP_TYPE > heapType;
    if( params.mUsage == Usage::Staging )
    {
      heapType = D3D12_HEAP_TYPE_UPLOAD;
    }
    else if( params.mUsage == Usage::Dynamic )
    {
      // Don't create a heap now, instead the context allocates a temporary when mapping
    }
    else
    {
      heapType = D3D12_HEAP_TYPE_DEFAULT;
    }

    const D3D12_HEAP_PROPERTIES HeapProps
    {
      .Type                 { heapType },
      .CPUPageProperty      { D3D12_CPU_PAGE_PROPERTY_UNKNOWN },
      .MemoryPoolPreference { D3D12_MEMORY_POOL_UNKNOWN },
      .CreationNodeMask     { 1 },
      .VisibleNodeMask      { 1 },
    };

    const  DXGI_SAMPLE_DESC SampleDesc
    {
      .Count   { 1 },
      .Quality { 0 },
    };

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
      .Flags            { D3D12_RESOURCE_FLAG_NONE },
    };

    const D3D12_RESOURCE_STATES DefaultUsage{ D3D12_RESOURCE_STATE_GENERIC_READ };

    ID3D12Device* device { mDevice };

    PCom< ID3D12Resource > buffer;
    TAC_DX12_CALL( device->CreateCommittedResource(
      &HeapProps,
      D3D12_HEAP_FLAG_NONE,
      &ResourceDesc,
      DefaultUsage,
      nullptr,
      buffer.iid(),
      buffer.ppv() ) );

    DX12SetName( buffer, sf );

    void* cpuAddr{};
    if( params.mUsage == Usage::Dynamic )
    {
      const UINT iSubresource{};
      TAC_DX12_CALL( buffer->Map(
        iSubresource, // subrsc idx
        nullptr, // D3D12_RANGE* nullptr indicates the whole subrsc may be read by cpu
        &cpuAddr ) );
    }

    const DescriptorBindings descriptorBindings{ CreateBindings( buffer.Get(), params.mBinding ) };

    const int i { h.GetIndex() };
    mBuffers[ i ] = DX12Buffer
    {
      .mResource      { buffer },
      .mDesc          { ResourceDesc },
      .mState         { DefaultUsage },
      .mSRV           { descriptorBindings.mSRV },
      .mUAV           { descriptorBindings.mUAV },
      .mMappedCPUAddr { cpuAddr },
    };
  }


  void DX12BufferMgr::UpdateBuffer( BufferHandle h,
                                    UpdateBufferParams params,
                                    DX12Context* context )
  {
    DX12Buffer& Buffer { mBuffers[ h.GetIndex() ] };
    TAC_ASSERT( Buffer.mMappedCPUAddr );
    char* dstBytes { ( char* )Buffer.mMappedCPUAddr + params.mDstByteOffset };
    MemCpy( dstBytes, params.mSrcBytes, params.mSrcByteCount );
  }

  void DX12BufferMgr::DestroyBuffer( BufferHandle h )
  {
    if( h.IsValid() )
      mBuffers[ h.GetIndex() ] = {};
  }
} // namespace Tac::Render
