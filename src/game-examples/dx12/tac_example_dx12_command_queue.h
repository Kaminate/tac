#pragma once

#include "src/common/tac_ints.h"
#include "src/shell/windows/tac_win32_com_ptr.h" // PCom

#include "tac_example_dx12_win32_event.h" // Win32Event
#include "tac_example_dx12_fence.h"

#include <d3d12.h> // ID3D12...

namespace Tac
{
  struct Errors;
}

namespace Tac::Render
{
  // Wrapper around ID3D12CommandQueue*
  // This is basically a singleton, there is (should be)
  // 1 Graphics CommandQueue
  // 1 Copy CommandQueue
  // 1 Compute CommandQueue
  struct DX12CommandQueue
  {

    void        Create(ID3D12Device*, Errors&);

    bool        IsFenceComplete( FenceSignal );
    FenceSignal ExecuteCommandList( ID3D12CommandList*, Errors& );
    void        WaitForFence( FenceSignal, Errors& );
    void        WaitForIdle( Errors& );
    FenceSignal IncrementFence(Errors& );
    FenceSignal GetLastCompletedFenceValue();

    ID3D12CommandQueue* GetCommandQueue() { return m_commandQueue.Get(); }
    
  private:
    //void   UpdateLastCompletedFenceValue(u64);
    void   CreateFence(ID3D12Device*, Errors&);
    void   CreateCommandQueue(ID3D12Device*, Errors&);
  public:

    // A fence is used to synchronize the CPU with the GPU (see Multi-engine synchronization).
    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization
    PCom< ID3D12Fence1 >               m_fence;

    // set on fence completion, then immediately waited on
    Win32Event                         m_fenceEvent;

    UINT64                             mLastCompletedFenceValue{0};
    UINT64                             mNextFenceValue{1};

    // A ID3D12CommandQueue provides methods for
    // - submitting command lists,
    // - synchronizing command list execution,
    // - instrumenting the command queue,
    // - etc
    //
    // Some examples:
    // - ID3D12CommandQueue::ExecuteCommandLists
    // - ID3D12CommandQueue::GetClockCalibration
    // - ID3D12CommandQueue::GetTimestampFrequency
    // - ID3D12CommandQueue::Signal
    // - ID3D12CommandQueue::Wait
    // 
    // Together, CommandLists/CommandQueues replace the ID3D11DeviceContext (?)
    //
    // tldr: A command queue can submit command lists
    PCom< ID3D12CommandQueue >         m_commandQueue;
  };
}
