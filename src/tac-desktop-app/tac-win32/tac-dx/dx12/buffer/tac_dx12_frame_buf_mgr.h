#pragma once

#include "tac_dx12_frame_buf.h"
//#include "tac-dx/dx12/tac_renderer_dx12_ver3.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/containers/tac_array.h"

#include <d3d12.h>

namespace Tac         { struct Errors; }
namespace Tac::Render { struct DX12DescriptorHeap; struct DX12TextureMgr; }

namespace Tac::Render
{

  struct DX12SwapChainMgr
  {
    SwapChainHandle CreateSwapChain( SwapChainParams, Errors& );
    void            ResizeSwapChain( SwapChainHandle, v2i, Errors& );
    auto            GetSwapChainParams( SwapChainHandle ) -> const SwapChainParams&;
    void            DestroySwapChain( SwapChainHandle );
    DX12SwapChain*  FindSwapChain( SwapChainHandle );
    TextureHandle   GetSwapChainCurrentColor( SwapChainHandle );
    TextureHandle   GetSwapChainDepth( SwapChainHandle );

  private:
    using DX12SwapChains = Array< DX12SwapChain, 100 >;
    TextureHandle       CreateDepthTexture( v2i, SwapChainParams , Errors& );
    DX12SwapChainImages CreateColorTextures( DXGISwapChainWrapper, SwapChainParams, Errors& );
    void                NameRTTextures( SwapChainHandle, DX12SwapChainImages, TextureHandle );

    DX12SwapChains      mSwapChains {};
  };
} // namespace Tac::Render
