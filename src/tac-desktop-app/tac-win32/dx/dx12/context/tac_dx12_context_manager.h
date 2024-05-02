#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_optional.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-win32/dx/dx12/context/tac_dx12_context.h"
#include "tac-win32/dx/dx12/tac_dx12_gpu_upload_allocator.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-rhi/render3/tac_render_backend.h"

#include <d3d12.h> // ID3D12...

namespace Tac { struct Errors; }

namespace Tac::Render
{
  struct DX12CommandAllocatorPool;
  struct DX12ContextManager;
  struct DX12CommandQueue;
  struct DX12SwapChainMgr;
  struct DX12TextureMgr;
  //struct DX12Device;
  //struct DX12Renderer;
  struct DX12BufferMgr;
  struct DX12PipelineMgr;

  // a contextmanager manages contexts
  struct DX12ContextManager
  {
    struct Params
    {
      DX12CommandAllocatorPool* mCommandAllocatorPool {};
      DX12CommandQueue*         mCommandQueue         {};
      DX12UploadPageMgr*        mUploadPageManager    {};
      DX12SwapChainMgr*         mSwapChainMgr         {};
      DX12TextureMgr*           mTextureMgr           {};
      DX12BufferMgr*            mBufferMgr            {};
      DX12PipelineMgr*          mPipelineMgr          {};
      ID3D12Device*             mDevice               {};
    };
    void Init( Params );

    DX12Context*                      GetContext( Errors& );
    void                              RetireContext( DX12Context* );
    PCom< ID3D12GraphicsCommandList > CreateCommandList( Errors& );

  private:
    Vector< DX12Context* >         mAvailableContexts;

    // singletons
    DX12CommandAllocatorPool* mCommandAllocatorPool {};
    DX12CommandQueue*         mCommandQueue         {};
    DX12UploadPageMgr*        mUploadPageManager    {};
    DX12SwapChainMgr*         mSwapChainMgr         {};
    DX12TextureMgr*           mTextureMgr           {};
    DX12BufferMgr*            mBufferMgr            {};
    DX12PipelineMgr*          mPipelineMgr          {};

    PCom< ID3D12Device4 >     mDevice;
    //DX12Renderer*             mRenderer{};
  };
}
