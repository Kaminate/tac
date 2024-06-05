#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap_allocation.h"
#include "tac-dx/dxgi/tac_dxgi.h"
#include "tac-dx/dx12/tac_dx12_command_queue.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-rhi/render3/tac_render_api.h"

#include <d3d12.h> // D3D12...

namespace Tac::Render
{
  const int TAC_SWAP_CHAIN_BUF_COUNT = 3;

#if 0
  struct DX12SwapChainImage
  {
    PCom< ID3D12Resource >       mResource;
    D3D12_RESOURCE_DESC          mDesc{};
    D3D12_RESOURCE_STATES        mState{};
    DX12DescriptorHeapAllocation mRTV;
  };
  using DX12SwapChainImages = Array< DX12SwapChainImage, TAC_SWAP_CHAIN_BUF_COUNT >;
#else
  using DX12SwapChainImages = Vector< TextureHandle >;
#endif

  struct DX12SwapChain
  {
    const void*             mNWH{};
    v2i                     mSize{};
    DXGISwapChainWrapper    mDXGISwapChain;
    DXGI_SWAP_CHAIN_DESC1   mDXGISwapChainDesc;
    DX12SwapChainImages     mRTColors;
    TextureHandle           mRTDepth;
    SwapChainParams         mSwapChainParams{};
  };
} // namespace Tac::Render
