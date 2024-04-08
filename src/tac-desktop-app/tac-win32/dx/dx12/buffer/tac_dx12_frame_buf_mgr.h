#pragma once

#include "tac_dx12_frame_buf.h"
//#include "tac-win32/dx/dx12/tac_renderer_dx12_ver3.h"
#include "tac-rhi/render3/tac_render_api.h"

#include <d3d12.h>

namespace Tac { struct Errors; }

namespace Tac::Render
{
  struct DX12DescriptorHeap;
  struct DX12FrameBufferMgr
  {
    void Init( ID3D12Device*,
               DX12CommandQueue*,
               DX12DescriptorHeap* );

    void   CreateFB( FBHandle, FrameBufferParams, Errors& );
    void   ResizeFB( FBHandle, v2i );
    TexFmt GetFBFmt( FBHandle );
    void   DestroyFB( FBHandle);

    DX12FrameBuf        mFrameBufs[ 100 ]{};
    ID3D12Device*       mDevice{};
    DX12DescriptorHeap* mCpuDescriptorHeapRTV{};
    DX12CommandQueue*   mCommandQueue{};
  };
} // namespace Tac::Render
