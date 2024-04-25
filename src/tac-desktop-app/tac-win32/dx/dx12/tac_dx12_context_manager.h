#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-std-lib/containers/tac_vector.h"
#include "tac_dx12_gpu_upload_allocator.h"
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

  // A context has a commandlist, even if the context is recycled, the commandlist stays with it
  // forever.
  //
  // However, the commandallocator is changed every time the context is recycled
  struct DX12Context : public IContext
  {
    DX12Context() = default;
#if 0
    DX12Context( DX12CommandAllocatorPool*,
                 DX12ContextManager*,
                 DX12CommandQueue*,
                 Errors* );
    ~DX12Context() override;
    DX12Context( DX12Context&& ) noexcept;
    void operator = ( DX12Context& ) = delete;
    void operator = ( DX12Context&& ) noexcept;
#endif

    // note(n473): i dont like how with dx12context::Begin and dx12context::Finish,
    // there is no protection (afaict) to prevent someone from forgetting to call Finish.
    ID3D12GraphicsCommandList* GetCommandList();
    ID3D12CommandAllocator*    GetCommandAllocator();
    void                       SetName( StringView );
    void                       Reset( Errors& );

    void                       Execute( Errors& ) override;
    void                       SetSynchronous() override;
    void                       SetViewport( v2i ) override;
    void                       SetScissor( v2i ) override;
    void                       SetRenderTargets( Targets ) override;

    void                       DebugEventBegin( StringView ) override;
    void                       DebugEventEnd() override;
    void                       DebugMarker( StringView ) override;
    void                       MoveFrom( DX12Context&& ) noexcept;

    void Retire() override;

    PCom< ID3D12GraphicsCommandList > mCommandList          {};
    PCom< ID3D12CommandAllocator >    mCommandAllocator     {};

    // ok so like this needs to be owned so different command lists dont mix up their upload memory
    DX12UploadAllocator               mGPUUploadAllocator   {};
    bool                              mSynchronous          {};
    bool                              mExecuted             {};

    // singletons
    DX12CommandAllocatorPool*         mCommandAllocatorPool {};
    DX12ContextManager*               mContextManager       {};
    DX12CommandQueue*                 mCommandQueue         {};
    DX12SwapChainMgr*                 mFrameBufferMgr       {};
    DX12TextureMgr*                   mTextureMgr           {};
    int                               mEventCount           {};
  };

  // a contextmanager manages contexts
  struct DX12ContextManager
  {
    void Init( DX12CommandAllocatorPool*,
               DX12CommandQueue*,
               DX12UploadPageMgr*,
               DX12SwapChainMgr*,
               ID3D12Device* );

    DX12Context*                      GetContext( Errors& );
    void                              RetireContext( DX12Context* );
    PCom< ID3D12GraphicsCommandList > CreateCommandList( Errors& );

  private:
    Vector< DX12Context* >         mAvailableContexts;

    // singletons
    DX12CommandAllocatorPool* mCommandAllocatorPool = nullptr;
    DX12CommandQueue*         mCommandQueue = nullptr;
    DX12UploadPageMgr*        mUploadPageManager = nullptr;
    DX12SwapChainMgr*       mFrameBufferMgr{};

    PCom< ID3D12Device4 >     mDevice;

  };
}
