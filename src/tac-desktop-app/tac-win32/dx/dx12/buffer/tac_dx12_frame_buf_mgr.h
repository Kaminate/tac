#pragma once

#include "tac_dx12_frame_buf.h"
//#include "tac-win32/dx/dx12/tac_renderer_dx12_ver3.h"
#include "tac-rhi/render3/tac_render_api.h"

#include <d3d12.h>

namespace Tac         { struct Errors; }
namespace Tac::Render { struct DX12DescriptorHeap; struct DX12TextureMgr; }

namespace Tac::Render
{

  struct DX12SwapChainMgr
  {
    struct Params
    {
      DX12TextureMgr* mTextureManager{};
      DX12CommandQueue* mCommandQueue{};
    };
    void Init( Params );

    void            CreateSwapChain( SwapChainHandle, SwapChainParams, Errors& );
    void            ResizeSwapChain( SwapChainHandle, v2i );
    SwapChainParams GetSwapChainParams( SwapChainHandle );
    void            DestroySwapChain( SwapChainHandle );
    DX12SwapChain*  FindSwapChain( SwapChainHandle );
    TextureHandle   GetSwapChainCurrentColor( SwapChainHandle );
    TextureHandle   GetSwapChainDepth( SwapChainHandle );

    DX12SwapChain       mSwapChains[ 100 ]    {};
    //ID3D12Device*       mDevice               {};
    //DX12DescriptorHeap* mCpuDescriptorHeapRTV {};
    DX12CommandQueue*   mCommandQueue         {};
    DX12TextureMgr*     mTextureMgr           {};
  };
} // namespace Tac::Render
