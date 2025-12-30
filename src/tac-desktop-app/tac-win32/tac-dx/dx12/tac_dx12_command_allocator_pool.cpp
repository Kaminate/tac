#include "tac_dx12_command_allocator_pool.h" // self-inc
#include "tac_dx12_command_queue.h"

#include "tac-dx/dx12/tac_dx12_helper.h" // TAC_DX12_CALL
#include "tac-dx/dx12/tac_renderer_dx12_ver3.h"

namespace Tac::Render
{


  void DX12CommandAllocatorPool::Retire( PCom< ID3D12CommandAllocator > allocator,
                                 FenceSignal signalVal )
  {
    mElements.Push(
      Element
      {
        .mCmdAllocator { allocator },
        .mSignalValue  { signalVal },
      } );
  }

  PCom< ID3D12CommandAllocator > DX12CommandAllocatorPool::CreateNewCmdAllocator( Errors& errors )
  {
    PCom< ID3D12CommandAllocator > allocator;

    ID3D12Device* device{ DX12Renderer::sRenderer.mDevice };
    TAC_ASSERT( device );

    // a command allocator manages storage for cmd lists and bundles
    TAC_DX12_CALL_RET( device->CreateCommandAllocator(
                       D3D12_COMMAND_LIST_TYPE_DIRECT,
                       allocator.iid(),
                       allocator.ppv() ) );

    DX12SetName( allocator.Get(), "Cmd allocator " + Tac::ToString( mAllocatorCount++ ) );

    return allocator;
  }

  PCom< ID3D12CommandAllocator > DX12CommandAllocatorPool::GetAllocator( Errors& errors )
  {
    DX12CommandQueue* commandQueue{ &DX12Renderer::sRenderer.mCommandQueue };
    FenceSignal fenceSignal { commandQueue->GetLastCompletedFenceValue() };
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

