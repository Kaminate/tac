#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-std-lib/containers/tac_vector.h"
#include "tac_example_dx12_gpu_upload_allocator.h"

#include <d3d12.h> // ID3D12...

namespace Tac
{
  struct Errors;
}

namespace Tac::Render
{
  struct DX12ExampleCommandAllocatorPool;
  struct DX12ExampleContextManager;
  struct DX12ExampleCommandQueue;

  // A context has a commandlist, even if the context is recycled, the commandlist stays with it
  // forever.
  //
  // However, the commandallocator is changed every time the context is recycled
  struct DX12ExampleContext
  {

    // note(n473): i dont like how with dx12context::Begin and dx12context::Finish,
    // there is no protection (afaict) to prevent someone from forgetting to call Finish.

    ID3D12GraphicsCommandList* GetCommandList() { return mCommandList.Get(); }

    PCom<ID3D12GraphicsCommandList> mCommandList;
    PCom<ID3D12CommandAllocator> mCommandAllocator;

    // ok so like this needs to be owned so different command lists dont mix up their upload memory
    DX12ExampleGPUUploadAllocator mGPUUploadAllocator;
  };

  struct DX12ExampleContextScope
  {
    DX12ExampleContextScope() = default;
    DX12ExampleContextScope( DX12ExampleContextScope&& ) noexcept;
    ~DX12ExampleContextScope();

    ID3D12GraphicsCommandList* GetCommandList();
    void                       ExecuteSynchronously();

    DX12ExampleContext         mContext;
    bool                       mSynchronous = false;

    // singletons
    DX12ExampleCommandAllocatorPool*  mCommandAllocatorPool = nullptr;
    DX12ExampleContextManager*        mContextManager = nullptr;
    DX12ExampleCommandQueue*          mCommandQueue = nullptr;
    Errors*                    mParentScopeErrors = nullptr;
    bool                       mMoved = false;
  };

  // a contextmanager manages contexts
  struct DX12ExampleContextManager
  {
    void Init( DX12ExampleCommandAllocatorPool*,
               DX12ExampleCommandQueue*,
               DX12ExampleGPUUploadPageManager*,
               PCom<ID3D12Device> );
    
    DX12ExampleContextScope                 GetContext( Errors& );
    void                             RetireContext( DX12ExampleContext context );
    
    PCom<ID3D12GraphicsCommandList > CreateCommandList(Errors&);

  private:
    Vector< DX12ExampleContext >     mAvailableContexts;

    // singletons
    DX12ExampleCommandAllocatorPool* mCommandAllocatorPool = nullptr;
    DX12ExampleCommandQueue*         mCommandQueue = nullptr;
    DX12ExampleGPUUploadPageManager*     mUploadPageManager = nullptr;
    PCom< ID3D12Device5 >     mDevice; // device4 needed for createcommandlist1
  };
}
