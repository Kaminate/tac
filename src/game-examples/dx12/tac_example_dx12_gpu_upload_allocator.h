#pragma once

#include <d3d12.h> // ID3D12...

#include "src/shell/windows/tac_win32_com_ptr.h" // PCom
#include "src/common/containers/tac_vector.h"

#include "tac_example_dx12_command_queue.h"

namespace Tac::Render
{

  struct GPUUploadPage
  {
    PCom<ID3D12Resource>      mBuffer;

    D3D12_GPU_VIRTUAL_ADDRESS mGPUAddr = 0;
    void*                     mCPUAddr = nullptr;
    int                       mByteCount = 0;


    static const int          kDefaultByteCount = 2 * 1024 * 1024;
  };

  struct GPUUploadPageManager
  {
    void Init( ID3D12Device* device, DX12CommandQueue* commandQueue )
    {
      m_device = device;
      mCommandQueue = commandQueue;
    };

    GPUUploadPage RequestPage( int byteCount, Errors& );
    void          UnretirePages();
    void          RetirePage( GPUUploadPage, FenceSignal );

  private:
    GPUUploadPage AllocateNewPage( int byteCount, Errors& );

    struct RetiredPage
    {
      GPUUploadPage mPage;
      FenceSignal mFence{};
    };

    Vector< RetiredPage > mRetiredPages;
    Vector< GPUUploadPage > mAvailablePages;

    // singletons
    PCom< ID3D12Device > m_device;
    DX12CommandQueue* mCommandQueue = nullptr;
  };

  struct GPUUploadAllocator
  {

    void Init( GPUUploadPageManager* pageManager )
    {
      mPageManager = pageManager;
    }

    struct DynAlloc
    {
      D3D12_GPU_VIRTUAL_ADDRESS mGPUAddr;
      void*                     mCPUAddr;
      int                       mByteCount;
    };


    DynAlloc Allocate( int const byteCount, Errors& );
    void     FreeAll( FenceSignal FenceID );

  private:

    int            mCurPageUsedByteCount = 0;

    // Currently in use by command queues the current frame, memory cannot be freed.
    // The last page is the current page
    Vector< GPUUploadPage > mActivePages;
    Vector< GPUUploadPage > mLargePages;

    // singletons
    GPUUploadPageManager* mPageManager = nullptr;
  };
}//namespace Tac::Render
