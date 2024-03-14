#include "tac_dx12_context_manager.h" // self-inc
#include "tac_dx12_command_allocator_pool.h"
#include "tac_dx12_gpu_upload_allocator.h"

#include "tac-win32/renderer/dx12/tac_dx12_helper.h"

#include "tac-std-lib/error/tac_error_handling.h"


namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  // DX12Context

  ID3D12GraphicsCommandList* DX12Context::GetCommandList() { return mCommandList.Get(); }

  void DX12Context::SetName( StringView name )
  {
    DX12SetName( mCommandAllocator, name );
    DX12SetName( mCommandList, name );
  }

  // -----------------------------------------------------------------------------------------------

  // DX12ContextScope

  DX12ContextScope::DX12ContextScope( DX12Context context,
                                      DX12CommandAllocatorPool* pool,
                                      DX12ContextManager* mgr,
                                      DX12CommandQueue* q,
                                      Errors* e )
  {
    mContext = context;
    mCommandAllocatorPool = mCommandAllocatorPool;
    mContextManager = mgr;
    mCommandQueue = mCommandQueue;
    mParentScopeErrors = e;
  }

  DX12ContextScope::DX12ContextScope( DX12ContextScope&& other ) noexcept
  {
    MoveFrom( ( DX12ContextScope&& )other );
  }

  DX12ContextScope::~DX12ContextScope()
  {
    if( mMoved )
      return;

    Errors& errors = *mParentScopeErrors;

    ID3D12GraphicsCommandList* commandList = mContext.GetCommandList();
    
    // Indicates that recording to the command list has finished.
    TAC_DX12_CALL( commandList->Close() );

    const FenceSignal fenceSignal = TAC_CALL(
      mCommandQueue->ExecuteCommandList( commandList, errors ) );

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

  ID3D12GraphicsCommandList* DX12ContextScope::GetCommandList()       { return mContext.GetCommandList(); }
  void                       DX12ContextScope::ExecuteSynchronously() { mSynchronous = true; }

  void DX12ContextScope::MoveFrom( DX12ContextScope&& other ) noexcept
  {
    other.mMoved = true;

    mContext = other.mContext;
    mSynchronous = other.mSynchronous;
    mCommandAllocatorPool = other.mCommandAllocatorPool;
    mContextManager = other.mContextManager;
    mCommandQueue = other.mCommandQueue;
    mParentScopeErrors = other.mParentScopeErrors;
  }

  void DX12ContextScope::operator = ( DX12ContextScope&& other ) noexcept { other.mMoved = true; }

  // -----------------------------------------------------------------------------------------------

  // DX12ContextManager

  void DX12ContextManager::RetireContext( DX12Context context )
  {
    TAC_ASSERT( !context.mCommandAllocator );
    mAvailableContexts.push_back( context );
  }

  PCom< ID3D12GraphicsCommandList > DX12ContextManager::CreateCommandList( Errors& errors )
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

  void DX12ContextManager::Init( DX12CommandAllocatorPool* commandAllocatorPool,
                                 DX12CommandQueue* commandQueue,
                                 DX12UploadPageMgr* uploadPageManager,
                                 ID3D12Device* device )
  {
    mCommandAllocatorPool = commandAllocatorPool;
    mCommandQueue = commandQueue;
    mUploadPageManager = uploadPageManager;
    device->QueryInterface( mDevice.iid(), mDevice.ppv() );
    TAC_ASSERT( mDevice );
  }

  DX12Context DX12ContextManager::GetContextNoScope( Errors& errors )
  {
    DX12Context context;

    if( mAvailableContexts.empty() )
    {
      PCom<ID3D12GraphicsCommandList > cmdList = TAC_CALL_RET( {}, CreateCommandList( errors ) );
      context.mCommandList = cmdList;
      context.mGPUUploadAllocator.Init( mUploadPageManager );
    }
    else
    {
      context = mAvailableContexts.back();
      mAvailableContexts.pop_back();
    }

    context.mCommandAllocator = 
        TAC_CALL_RET( {}, mCommandAllocatorPool->GetAllocator( errors ) );

    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    //
    // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandallocator-reset
    // ID3D12CommandAllocator::Reset
    //   Indicates to re-use the memory that is associated with the command allocator.
    //   From this call to Reset, the runtime and driver determine that the GPU is no longer
    //   executing any command lists that have recorded commands with the command allocator.
    ID3D12CommandAllocator* dxCommandAllocator = context.mCommandAllocator.Get();
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
    ID3D12GraphicsCommandList* dxCommandList = context.GetCommandList();
    TAC_DX12_CALL_RET( {}, dxCommandList->Reset( dxCommandAllocator, nullptr ) );

    return context;
  }

  DX12ContextScope DX12ContextManager::GetContext( Errors& errors )
  {
    DX12Context context = TAC_CALL_RET( {}, GetContextNoScope( errors ) );
    DX12ContextScope scope( context,
                            mCommandAllocatorPool,
                            this,
                            mCommandQueue,
                            &errors );
    return scope;
  }

}
