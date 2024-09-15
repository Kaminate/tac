#pragma once

#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap_allocation.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap_gpu_mgr.h"
#include "tac-std-lib/containers/tac_span.h"

#include <d3d12.h> // ID3D12...

namespace Tac::Render
{
  struct DX12DescriptorCache
  {
    void                  SetRegionManager( DX12DescriptorRegionManager* );
    void                  SetFence( FenceSignal );
    void                  Clear();
    DX12DescriptorRegion* GetGPUDescriptorForCPUDescriptors( Span< DX12Descriptor > );

  private:
    DX12DescriptorRegion* Lookup( DX12Descriptor );

    Vector< DX12Descriptor >        mCPUDescs;
    Vector< int >                   mGPUIndexes;
    Vector< DX12DescriptorRegion >  mGPUDescs;
    DX12DescriptorRegionManager*    mGpuRegionMgr{};
  };

  using DX12DescriptorCaches = DX12DescriptorCache[ D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES ];
}
