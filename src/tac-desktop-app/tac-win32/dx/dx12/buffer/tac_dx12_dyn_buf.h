#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-win32/dx/dx12/descriptor/tac_dx12_descriptor_heap_allocation.h"

#include <d3d12.h> // D3D12...

namespace Tac::Render
{
  struct DX12Buffer
  {
    PCom< ID3D12Resource >         mResource;
    D3D12_RESOURCE_DESC            mDesc{};
    D3D12_RESOURCE_STATES          mState{};
    DX12DescriptorHeapAllocation   mRTV;
    void*                          mMappedCPUAddr {};
  };

} // namespace Tac::Render
