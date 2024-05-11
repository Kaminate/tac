#pragma once

#include "tac-win32/dx/dx12/sampler/tac_dx12_sampler.h"
#include "tac-win32/dx/dx12/descriptor/tac_dx12_descriptor_heap.h"
#include "tac-rhi/render3/tac_render_api.h"

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
    void         CreateSampler( SamplerHandle, Filter );
    void         DestroySampler( SamplerHandle );
    DX12Sampler* FindSampler( SamplerHandle );

  private:

    DX12Sampler         mSamplers[ 100 ];
    ID3D12Device*       mDevice{};
    DX12DescriptorHeap* mCpuDescriptorHeapSampler{};
  };
} // namespace Tac::Render

