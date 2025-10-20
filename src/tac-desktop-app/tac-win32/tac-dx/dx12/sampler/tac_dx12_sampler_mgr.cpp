#include "tac_dx12_sampler_mgr.h" // self-inc
#include "tac-rhi/render3/tac_render_backend.h"
#include "tac-dx/dx12/tac_renderer_dx12_ver3.h"


namespace Tac::Render
{

  static D3D12_FILTER GetFilter( Filter filter )
  {
    switch( filter )
    {
    case Filter::Point: return D3D12_FILTER_MIN_MAG_MIP_POINT;
    case Filter::Linear: return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    case Filter::Aniso: return D3D12_FILTER_ANISOTROPIC;
    }

    return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  }

  // -----------------------------------------------------------------------------------------------


  DX12Sampler* DX12SamplerMgr::FindSampler( SamplerHandle h )
  {
    return h.IsValid() ? &mSamplers[ h.GetIndex() ] : nullptr;
  }

  void DX12SamplerMgr::DestroySampler( SamplerHandle h )
  {
    if( h.IsValid() )
    {
      FreeHandle( h );
      mSamplers[ h.GetIndex() ] = {};
    }
  }

  SamplerHandle DX12SamplerMgr::CreateSampler( CreateSamplerParams params )
  {
    DX12Renderer&          renderer{ DX12Renderer::sRenderer };
    DX12DescriptorHeapMgr& heapMgr { renderer.mDescriptorHeapMgr };
    DX12DescriptorHeap&    heap    { heapMgr.mCPUHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ] };
    ID3D12Device*          device  { renderer.mDevice };

    const D3D12_FILTER dx12filter{ GetFilter( params.mFilter ) };
    const D3D12_SAMPLER_DESC Desc
    {
      .Filter         { dx12filter },
      .AddressU       { D3D12_TEXTURE_ADDRESS_MODE_WRAP },
      .AddressV       { D3D12_TEXTURE_ADDRESS_MODE_WRAP },
      .AddressW       { D3D12_TEXTURE_ADDRESS_MODE_WRAP },
      .ComparisonFunc { D3D12_COMPARISON_FUNC_NEVER },
      .MinLOD         {},
      .MaxLOD         { D3D12_FLOAT32_MAX },
    };
    const DX12Descriptor descriptor{ heap.Allocate( params.mName ) };
    const D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle { descriptor.GetCPUHandle() };
    const SamplerHandle h{ AllocSamplerHandle() };
    device->CreateSampler( &Desc, descriptorHandle );
    mSamplers[ h.GetIndex() ] = DX12Sampler
    {
      .mDescriptor{ descriptor },
      .mName      { params.mName },
    };
    return h;
  }
} // namespace Tac::Render

