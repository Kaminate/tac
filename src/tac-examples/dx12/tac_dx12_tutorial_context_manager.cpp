#include "tac_dx12_tutorial_context_manager.h" // self-inc
#include "tac_dx12_tutorial_command_allocator_pool.h"
#include "tac_dx12_tutorial_gpu_upload_allocator.h"

#include "tac-dx/dx12/tac_dx12_helper.h"

#include "tac-std-lib/error/tac_error_handling.h"


namespace Tac::Render
{

  DX12ExampleContextScope::~DX12ExampleContextScope()
  {
    if( mMoved )
      return;

    Errors& errors = *mParentScopeErrors;

    ID3D12GraphicsCommandList* commandList = mContext.GetCommandList();
    
    // Indicates that recording to the command list has finished.
    TAC_DX12_CALL( commandList->Close() );

    FenceSignal fenceSignal = TAC_CALL( mCommandQueue->ExecuteCommandList( commandList, errors ) );

    if( mSynchronous )
    {
      mCommandQueue->WaitForFence( fenceSignal, errors );
      TAC_ASSERT( !errors );
    }

    mCommandAllocatorPool->Retire( mContext.mCommandAllocator, fenceSignal );
    mContext.mCommandAllocator = {};
    mContext.mGPUUploadAllocator.FreeAll( fenceSignal );

    mContextManager->RetireContext( mContext );
  }

  ID3D12GraphicsCommandList* DX12ExampleContextScope::GetCommandList()
  {
    return mContext.GetCommandList();
  }

  void DX12ExampleContextScope::ExecuteSynchronously()
  {
    mSynchronous = true;
  }

  // -----------------------------------------------------------------------------------------------

  void DX12ExampleContextManager::RetireContext( DX12ExampleContext context )
  {
    TAC_ASSERT( !context.mCommandAllocator );
    mAvailableContexts.push_back( context );
  }


  PCom<ID3D12GraphicsCommandList > DX12ExampleContextManager::CreateCommandList( Errors& errors )
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

    ID3D12CommandList* pCommandList{ commandList.Get() };
    DX12SetName( pCommandList, "My Command List" );

    PCom< ID3D12GraphicsCommandList > graphicsList =
      commandList.QueryInterface< ID3D12GraphicsCommandList >();

    TAC_ASSERT( graphicsList );
    return graphicsList;
  }

  void DX12ExampleContextManager::Init( DX12ExampleCommandAllocatorPool* commandAllocatorPool,
                                        DX12ExampleCommandQueue* commandQueue,
                                        DX12ExampleGPUUploadPageManager* uploadPageManager,
                                        PCom< ID3D12Device > device )
  {
    mCommandAllocatorPool = commandAllocatorPool;
    mCommandQueue = commandQueue;
    mUploadPageManager = uploadPageManager;
    mDevice = device.QueryInterface<ID3D12Device5>();
  }

  DX12ExampleContextScope DX12ExampleContextManager::GetContext( Errors& errors )
  {
    DX12ExampleContext context;

    if( mAvailableContexts.empty() )
    {
      PCom<ID3D12GraphicsCommandList > cmdList = TAC_CALL_RET( {}, CreateCommandList( errors ) );
      context.mCommandList = cmdList;
      context.mGPUUploadAllocator.Init(mUploadPageManager);
    }
    else
    {
      context = mAvailableContexts.back();
      mAvailableContexts.pop_back();
    }

    context.mCommandAllocator = 
        TAC_CALL_RET( {}, mCommandAllocatorPool->GetAllocator( errors ) );

    ID3D12GraphicsCommandList* dxCommandList { context.GetCommandList() };
    ID3D12CommandAllocator* dxCommandAllocator { context.mCommandAllocator.Get() };

    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    //
    // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandallocator-reset
    // ID3D12CommandAllocator::Reset
    //   Indicates to re-use the memory that is associated with the command allocator.
    //   From this call to Reset, the runtime and driver determine that the GPU is no longer
    //   executing any command lists that have recorded commands with the command allocator.
    TAC_DX12_CALL_RET( {}, dxCommandAllocator->Reset() );


    // However( when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    //
    // ID3D12GraphicsCommandList::Reset
    //
    //   Resets a command list back to its initial state as if a new command list was just created.
    //   After Reset succeeds, the command list is left in the "recording" state.
    //
    //   you can re-use command list tracking structures without any allocations
    //   you can call Reset while the command list is still being executed
    //   you can submit a cmd list, reset it, and reuse the allocated memory for another cmd list
    TAC_DX12_CALL_RET( {}, dxCommandList->Reset( dxCommandAllocator, nullptr ) );

    DX12ExampleContextScope scope;
    scope.mContext = context;
    scope.mCommandAllocatorPool = mCommandAllocatorPool;
    scope.mContextManager = this;
    scope.mCommandQueue = mCommandQueue;
    scope.mParentScopeErrors = &errors;

    return scope;
  }

  DX12ExampleContextScope::DX12ExampleContextScope( DX12ExampleContextScope&& other ) noexcept
  {
    other.mMoved = true;

    mContext = other.mContext;
    mSynchronous = other.mSynchronous;
    mCommandAllocatorPool = other.mCommandAllocatorPool;
    mContextManager = other.mContextManager;
    mCommandQueue = other.mCommandQueue;
    mParentScopeErrors = other.mParentScopeErrors;
  }
}
