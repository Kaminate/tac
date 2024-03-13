#include "tac_dx12_command_queue.h"

#include "src/shell/windows/renderer/dx12/tac_dx12_helper.h" // TAC_DX12_CALL_RET
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  FenceSignal DX12CommandQueue::ExecuteCommandList( ID3D12CommandList* cmdlist, Errors& errors )
  {
    const Array cmdLists{ cmdlist };

    // Submits an array of command lists for execution.
    m_commandQueue->ExecuteCommandLists( ( UINT )cmdLists.size(), cmdLists.data() );

    return IncrementFence( errors );
  }

  FenceSignal DX12CommandQueue::GetLastCompletedFenceValue()
  {
    u64 val = m_fence->GetCompletedValue();
    mLastCompletedFenceValue = val;
    return val;
  }

  FenceSignal DX12CommandQueue::IncrementFence( Errors& errors )
  {
    const UINT64 signalValue = mNextFenceValue;

    // Use this method to set a fence value from the GPU side
    TAC_DX12_CALL_RET( {}, m_commandQueue->Signal( ( ID3D12Fence* )m_fence, mNextFenceValue ) );
    mNextFenceValue++;
    return { signalValue };
  }

  void DX12CommandQueue::WaitForFence( FenceSignal signalValue, Errors& errors )
  {
    if( IsFenceComplete( signalValue ) )
      return;

    // ID3D12Fence::GetCompletedValue
    // - Gets the current value of the fence.
    //
    // Wait until the previous frame is finished.

    // I think this if statement is used because the alternative
    // would be while( m_fence->GetCompletedValue() != fence ) { TAC_NO_OP; }
    //const UINT64 curValue = m_fence->GetCompletedValue();
    //if( curValue < signalValue.mValue )
    //{
    // m_fenceEvent is only ever used in this scope 

    // ID3D12Fence::SetEventOnCompletion
    // - Specifies an event that's raised when the fence reaches a certain value.
    //
    // the event will be 'complete' when it reaches the specified value.
    // This value is set by the cmdqueue::Signal
    const u64 val = signalValue.GetValue();
    TAC_DX12_CALL( m_fence->SetEventOnCompletion( val, ( HANDLE )m_fenceEvent ) );
    WaitForSingleObject( ( HANDLE )m_fenceEvent, INFINITE );
    mLastCompletedFenceValue = val;
    UINT64 completedValue = m_fence->GetCompletedValue();
    TAC_ASSERT( mLastCompletedFenceValue == completedValue );
  }

  //void DX12CommandQueue::UpdateLastCompletedFenceValue(u64 val)
  //{
  //  if( val < mLastCompletedFenceValue )
  //    return;

  //  const u64 curFenceValue = m_fence->GetCompletedValue();
  //  mLastCompletedFenceValue = Max( mLastCompletedFenceValue, curFenceValue );
  //}

  bool DX12CommandQueue::IsFenceComplete( FenceSignal fenceValue )
  {
    const u64 val = fenceValue.GetValue();
    if( val <= mLastCompletedFenceValue )
      return true;

    mLastCompletedFenceValue = m_fence->GetCompletedValue();
    return val <= mLastCompletedFenceValue;
  }

  void DX12CommandQueue::Create( ID3D12Device* device, Errors& errors )
  {
    TAC_CALL( CreateFence( device, errors ) );
    TAC_CALL( CreateCommandQueue( device, errors ) );
  }

  void DX12CommandQueue::CreateFence( ID3D12Device* device, Errors& errors )
  {
    // Create synchronization objects.

    PCom< ID3D12Fence > fence;
    TAC_DX12_CALL( device->CreateFence(
      mLastCompletedFenceValue, // initial value
      D3D12_FENCE_FLAG_NONE,
      fence.iid(),
      fence.ppv() ) );

    fence.QueryInterface( m_fence );
    DX12SetName( fence, "fence" );

    TAC_CALL( m_fenceEvent.Init( errors ) );
  }

  void DX12CommandQueue::CreateCommandQueue( ID3D12Device* device, Errors& errors )
  {
    const D3D12_COMMAND_QUEUE_DESC queueDesc
    {
      // Specifies a command buffer that the GPU can execute.
      // A direct command list doesn't inherit any GPU state.
      // [ ] Q: 
      // [ ] A(?): ( As opposed to a bundle command list, which does )
      //
      // [ ] Q: What GPU state does a bundle command list inherit?
      // [ ] A: 

      // This command queue manages direct command lists (direct = for graphics rendering)
      .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
    };

    TAC_DX12_CALL( device->CreateCommandQueue(
      &queueDesc,
      m_commandQueue.iid(),
      m_commandQueue.ppv() ) );
    DX12SetName( m_commandQueue, "Command Queue" );
  }


  void DX12CommandQueue::WaitForIdle( Errors& errors )
  {
    FenceSignal fenceValue = TAC_CALL( IncrementFence( errors ) );
    TAC_CALL( WaitForFence( fenceValue, errors ) );
  };
}
