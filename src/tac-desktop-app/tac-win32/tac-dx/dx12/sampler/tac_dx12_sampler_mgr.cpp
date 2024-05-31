#include "tac_dx12_sampler_mgr.h" // self-inc


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

  void DX12SamplerMgr::Init( Params params )
  {
    mDevice = params.mDevice;
    mCpuDescriptorHeapSampler = params.mCpuDescriptorHeapSampler;
    TAC_ASSERT( mDevice && mCpuDescriptorHeapSampler );
  }

  DX12Sampler* DX12SamplerMgr::FindSampler( SamplerHandle h )
  {
    return h.IsValid() ? &mSamplers[ h.GetIndex() ] : nullptr;
  }

  void DX12SamplerMgr::DestroySampler( SamplerHandle h )
  {
    if( h.IsValid() )
      mSamplers[ h.GetIndex() ] = {};
  }

  void DX12SamplerMgr::CreateSampler( SamplerHandle h,
                                      CreateSamplerParams params )
  {
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

    const DX12Descriptor descriptor{ mCpuDescriptorHeapSampler->Allocate() };
    const D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle { descriptor.GetCPUHandle() };

    mDevice->CreateSampler( &Desc, descriptorHandle );
  
    mSamplers[ h.GetIndex() ] = DX12Sampler
    {
      .mDescriptor{ descriptor },
      .mName{params.mName},
    };

  }
} // namespace Tac::Render

