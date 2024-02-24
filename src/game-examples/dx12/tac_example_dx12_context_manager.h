#pragma once

#include "src/shell/windows/tac_win32_com_ptr.h" // PCom
#include "src/common/containers/tac_vector.h"
#include "tac_example_dx12_gpu_upload_allocator.h"

#include <d3d12.h> // ID3D12...

namespace Tac
{
  struct Errors;
}

namespace Tac::Render
{
  struct DX12CommandAllocatorPool;
  struct DX12ContextManager;
  struct DX12CommandQueue;

  // A context has a commandlist, even if the context is recycled, the commandlist stays with it
  // forever.
  //
  // However, the commandallocator is changed every time the context is recycled
  struct DX12Context
  {

    // note(n473): i dont like how with dx12context::Begin and dx12context::Finish,
    // there is no protection (afaict) to prevent someone from forgetting to call Finish.

    ID3D12GraphicsCommandList* GetCommandList() { return mCommandList.Get(); }

    PCom<ID3D12GraphicsCommandList> mCommandList;
    PCom<ID3D12CommandAllocator> mCommandAllocator;

    // ok so like this needs to be owned so different command lists dont mix up their upload memory
    GPUUploadAllocator mGPUUploadAllocator;
  };

  struct DX12ContextScope
  {
    DX12ContextScope() = default;
    DX12ContextScope( DX12ContextScope&& ) noexcept;
    ~DX12ContextScope();

    ID3D12GraphicsCommandList* GetCommandList();
    void                       ExecuteSynchronously();

    DX12Context                mContext;
    bool                       mSynchronous = false;

    // singletons
    DX12CommandAllocatorPool*  mCommandAllocatorPool = nullptr;
    DX12ContextManager*        mContextManager = nullptr;
    DX12CommandQueue*          mCommandQueue = nullptr;
    Errors*                    mParentScopeErrors = nullptr;
    bool                       mMoved = false;
  };

  // a contextmanager manages contexts
  struct DX12ContextManager
  {
    void Init( DX12CommandAllocatorPool*,
               DX12CommandQueue*,
               GPUUploadPageManager*,
               PCom<ID3D12Device> );
    
    DX12ContextScope                 GetContext( Errors& );
    void                             RetireContext( DX12Context context );
    
    PCom<ID3D12GraphicsCommandList > CreateCommandList(Errors&);

  private:
    Vector< DX12Context >     mAvailableContexts;

    // singletons
    DX12CommandAllocatorPool* mCommandAllocatorPool = nullptr;
    DX12CommandQueue*         mCommandQueue = nullptr;
    GPUUploadPageManager*     mUploadPageManager = nullptr;
    PCom< ID3D12Device5 >     mDevice; // device4 needed for createcommandlist1
  };
}
