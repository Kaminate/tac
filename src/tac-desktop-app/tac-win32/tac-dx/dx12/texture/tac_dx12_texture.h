#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap_allocation.h"
#include "tac-dx/dx12/resource/tac_dx12_resource.h"
#include "tac-std-lib/containers/tac_optional.h"
#include "tac-rhi/render3/tac_render_api.h"

#include <d3d12.h> // D3D12...

namespace Tac::Render
{
  struct DX12Texture
  {
    DX12Resource                mResource {};
    Binding                     mBinding  { Binding::None };
    String                      mName     {}; // for debugging
    Optional< DX12Descriptor >  mRTV      {};
    Optional< DX12Descriptor >  mDSV      {};
    Optional< DX12Descriptor >  mSRV      {};
    Optional< DX12Descriptor >  mUAV      {};
  };

} // namespace Tac::Render
