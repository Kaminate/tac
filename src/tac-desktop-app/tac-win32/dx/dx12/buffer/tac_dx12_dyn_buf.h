#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-win32/dx/dx12/descriptor/tac_dx12_descriptor_heap_allocation.h"
#include "tac-std-lib/containers/tac_optional.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-rhi/render3/tac_render_api.h"

#include <d3d12.h> // D3D12...

namespace Tac::Render
{
  struct DX12Buffer
  {
    using SRV = Optional< DX12Descriptor >;
    using UAV = Optional< DX12Descriptor >;

    PCom< ID3D12Resource >    mResource       {};
    D3D12_RESOURCE_DESC       mDesc           {};
    D3D12_RESOURCE_STATES     mState          {};
    D3D12_GPU_VIRTUAL_ADDRESS mGPUVirtualAddr {}; // used in D3D12_VERTEX_BUFFER_VIEW
    SRV                       mSRV            {};
    UAV                       mUAV            {};
    void*                     mMappedCPUAddr  {};
    CreateBufferParams        mCreateParams   {};
    String                    mCreateName     {};
  };

} // namespace Tac::Render
