#include "tac_dx12_tutorial_command_allocator_pool.h" // self-inc

#include "tac_dx12_tutorial_command_queue.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h" // TAC_DX12_CALL

namespace Tac::Render
{

  void DX12ExampleCommandAllocatorPool::Init( PCom<ID3D12Device > device,
                                         DX12ExampleCommandQueue* commandQueue )
    {
      mCommandQueue = commandQueue;
      m_device = device.QueryInterface<ID3D12Device5>();
    }

  void DX12ExampleCommandAllocatorPool::Retire( PCom< ID3D12CommandAllocator > allocator,
                                 FenceSignal signalVal )
  {
    Element element
    {
      .mCmdAllocator { allocator },
      .mSignalValue { signalVal },
    };

    mElements.Push( element );
  }

  PCom< ID3D12CommandAllocator > DX12ExampleCommandAllocatorPool::CreateNewCmdAllocator( Errors& errors )
  {
    PCom< ID3D12CommandAllocator > allocator;

    // a command allocator manages storage for cmd lists and bundles
    TAC_ASSERT( m_device );
    TAC_DX12_CALL_RET( {},
                       m_device->CreateCommandAllocator(
                       D3D12_COMMAND_LIST_TYPE_DIRECT,
                       allocator.iid(),
                       allocator.ppv() ) );

    return allocator;
  }

  PCom< ID3D12CommandAllocator > DX12ExampleCommandAllocatorPool::GetAllocator( Errors& errors )
  {
    FenceSignal fenceSignal = mCommandQueue->GetLastCompletedFenceValue();
    return GetAllocator( fenceSignal, errors );
  }

  PCom< ID3D12CommandAllocator > DX12ExampleCommandAllocatorPool::GetAllocator(
    FenceSignal signalVal, Errors& errors )
  {
    if( PCom< ID3D12CommandAllocator > allocator{ TryReuseAllocator( signalVal ) }  )
      return allocator;

    return CreateNewCmdAllocator(errors);
  }

  PCom< ID3D12CommandAllocator > DX12ExampleCommandAllocatorPool::TryReuseAllocator(
    FenceSignal signalVal )
  {
    if( mElements.empty() )
      return {};

    Element& element { mElements.front() };
    if( signalVal < element.mSignalValue )
      return {};

    PCom< ID3D12CommandAllocator > result { element.mCmdAllocator };
    mElements.Pop();
    return result;
  }
} // namespace Tac::Render

