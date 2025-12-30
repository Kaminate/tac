#pragma once

#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap_allocation.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_allocator.h"
#include "tac-std-lib/containers/tac_span.h"

#include <d3d12.h> // ID3D12...

namespace Tac::Render
{
  struct DX12DescriptorCache
  {
    void SetFence( FenceSignal );
    void AddDescriptorRegion( DX12DescriptorRegion&& );
  private:
    Vector< DX12DescriptorRegion >  mGPUDescs     {};
  };

} // namespace Tac::Render
