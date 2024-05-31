#pragma once

#include "tac-dx/dx12/sampler/tac_dx12_sampler.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/containers/tac_array.h"

namespace Tac::Render
{
  struct DX12SamplerMgr
  {
    struct Params
    {
      ID3D12Device*       mDevice{};
      DX12DescriptorHeap* mCpuDescriptorHeapSampler{};
    };

    void         Init( Params );
    void         CreateSampler( SamplerHandle, CreateSamplerParams );
    void         DestroySampler( SamplerHandle );
    DX12Sampler* FindSampler( SamplerHandle );

  private:
    using DX12Samplers = Array< DX12Sampler, 100 >;

    DX12Samplers        mSamplers                 {};
    ID3D12Device*       mDevice                   {};
    DX12DescriptorHeap* mCpuDescriptorHeapSampler {};
  };
} // namespace Tac::Render

