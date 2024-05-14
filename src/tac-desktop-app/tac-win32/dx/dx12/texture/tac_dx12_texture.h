#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-win32/dx/dx12/descriptor/tac_dx12_descriptor_heap_allocation.h"
#include "tac-std-lib/containers/tac_optional.h"

#include <d3d12.h> // D3D12...

namespace Tac::Render
{
  struct DX12Texture
  {
    PCom< ID3D12Resource >      mResource;
    D3D12_RESOURCE_DESC         mDesc{};
    D3D12_RESOURCE_STATES       mState{};
    Optional< DX12Descriptor >  mRTV;
    Optional< DX12Descriptor >  mDSV;
    Optional< DX12Descriptor >  mSRV;
  };

} // namespace Tac::Render
