#pragma once

#include <d3d12.h> // ID3D12...

#include "src/shell/windows/tac_win32_com_ptr.h" // PCom
#include "src/common/containers/tac_vector.h"

#include "tac_example_dx12_command_queue.h"

namespace Tac::Render
{
  struct GPUUploadAllocator
  {

    struct DynAlloc
    {
      D3D12_GPU_VIRTUAL_ADDRESS mGPUAddr;
      void* mCPUAddr;
      int                       mByteCount;
    };

    struct Page
    {
      PCom<ID3D12Resource>      mBuffer;
      //D3D12_RESOURCE_STATES     DefaultUsage{ D3D12_RESOURCE_STATE_GENERIC_READ };

      D3D12_GPU_VIRTUAL_ADDRESS mGPUAddr = 0;
      void* mCPUAddr = nullptr;
      int                       mByteCount = 0;

      //bool IsValid() const { return mGPUAddr != 0; }

      static const int          kDefaultByteCount = 2 * 1024 * 1024;

    };

    struct RetiredPage
    {
      Page mPage;
      DX12CommandQueue::Signal mFence;
    };

    DynAlloc Allocate( int const byteCount, Errors& );

    void FreeAll( DX12CommandQueue::Signal FenceID );

  private:


    void UnretirePages();
    void RequestPage( int byteCount, Errors& );

    Page AllocateNewPage( int byteCount, Errors& );
    PCom< ID3D12Device > m_device;

    int            mCurPageUsedByteCount = 0;

    // Currently in use by command queues the current frame, memory cannot be freed.
    // The last page is the current page
    Vector< Page > mActivePages;

    // | at the end of each frame, active pages go into retired pages
    // V

    Vector< RetiredPage > mRetiredPages;

    // | when pages are no long used
    // V

    Vector< Page > mAvailablePages;

    DX12CommandQueue* mCommandQueue = nullptr;
  };
}//namespace Tac::Render
