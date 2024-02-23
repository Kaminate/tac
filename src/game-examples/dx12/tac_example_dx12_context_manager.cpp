#include "tac_example_dx12_context_manager.h" // self-inc
#include "tac_example_dx12_command_allocator_pool.h"
#include "tac_example_dx12_gpu_upload_allocator.h"

#include "src/shell/windows/renderer/dx12/tac_dx12_helper.h"

#include "src/common/error/tac_error_handling.h"


namespace Tac::Render
{

  DX12ContextScope::~DX12ContextScope()
  {
    Errors errors;
    FenceSignal fenceSignal = mCommandQueue->ExecuteCommandList( GetCommandList(), errors );
    TAC_ASSERT( !errors );

    if( mSynchronous )
    {
      mCommandQueue->WaitForFence( fenceSignal, errors );
      TAC_ASSERT( !errors );
    }

    mCommandAllocatorPool->Retire( mContext.mCommandAllocator, fenceSignal );
    mContext.mCommandAllocator = {};

    mGPUUploadAllocator->FreeAll( fenceSignal );


    mContextManager->RetireContext(mContext );
  }

  // -----------------------------------------------------------------------------------------------

  void DX12ContextManager::RetireContext( DX12Context context )
  {
    TAC_ASSERT( !context.mCommandAllocator );
    mAvailableContexts.push_back( context );
  }


  PCom<ID3D12GraphicsCommandList > DX12ContextManager::CreateCommandList( Errors& errors )
  {

    // Create the command list
    //
    // Note: CreateCommandList1 creates it the command list in a closed state, as opposed to
    //       CreateCommandList, which creates in a open state.
    PCom< ID3D12CommandList > commandList;
    TAC_DX12_CALL_RET( {}, mDevice->CreateCommandList1( 0,
                       D3D12_COMMAND_LIST_TYPE_DIRECT,
                       D3D12_COMMAND_LIST_FLAG_NONE,
                       commandList.iid(),
                       commandList.ppv() ) );
    TAC_ASSERT( commandList );

    PCom<ID3D12GraphicsCommandList > graphicsList =
      commandList.QueryInterface<ID3D12GraphicsCommandList>();

    TAC_ASSERT( graphicsList );
    DX12SetName( graphicsList, "My Command List" );
    return graphicsList;
  }

  DX12ContextScope DX12ContextManager::GetContext( Errors& errors )
  {
    DX12Context context;

    if( mAvailableContexts.empty() )
    {
      PCom<ID3D12GraphicsCommandList > cmdList = TAC_CALL_RET( {}, CreateCommandList( errors ) );
      context.mCommandList = cmdList;
    }
    else
    {
      context = mAvailableContexts.back();
      mAvailableContexts.pop_back();
    }

    context.mCommandAllocator = 
        TAC_CALL_RET( {}, mCommandAllocatorPool->GetAllocator( errors ) );

    TAC_ASSERT( context.mCommandList );
    TAC_ASSERT( context.mCommandAllocator );

    DX12ContextScope scope
    {
      .mContext = context,
      .mCommandAllocatorPool = mCommandAllocatorPool,
      .mContextManager = this,
      .mCommandQueue = mCommandQueue,
      .mGPUUploadAllocator = mGPUUploadAllocator,
    };

    return scope;
  }
}
