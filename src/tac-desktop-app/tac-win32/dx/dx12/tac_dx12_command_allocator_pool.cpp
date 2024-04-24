#include  "tac_dx12_command_allocator_pool.h" // self-inc
#include  "tac_dx12_command_queue.h"

#include "tac-win32/dx/dx12/tac_dx12_helper.h" // TAC_DX12_CALL

namespace Tac::Render
{

  void DX12CommandAllocatorPool::Init( ID3D12Device* device,
                                       DX12CommandQueue* commandQueue )
  {
    mCommandQueue = commandQueue;
    m_device = device;
  }

  void DX12CommandAllocatorPool::Retire( PCom< ID3D12CommandAllocator > allocator,
                                 FenceSignal signalVal )
  {
    const Element element
    {
      .mCmdAllocator { allocator },
      .mSignalValue { signalVal },
    };

    mElements.Push( element );
  }

  PCom< ID3D12CommandAllocator > DX12CommandAllocatorPool::CreateNewCmdAllocator( Errors& errors )
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

  PCom< ID3D12CommandAllocator > DX12CommandAllocatorPool::GetAllocator( Errors& errors )
  {
    FenceSignal fenceSignal { mCommandQueue->GetLastCompletedFenceValue() };
    return GetAllocator( fenceSignal, errors );
  }

  PCom< ID3D12CommandAllocator > DX12CommandAllocatorPool::GetAllocator(
    FenceSignal signalVal, Errors& errors )
  {
    if( PCom< ID3D12CommandAllocator > allocator{ TryReuseAllocator( signalVal ) }  )
      return allocator;

    return CreateNewCmdAllocator(errors);
  }

  PCom< ID3D12CommandAllocator > DX12CommandAllocatorPool::TryReuseAllocator(
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

