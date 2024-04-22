#include "tac_dx12_context_manager.h" // self-inc
#include "tac_dx12_command_allocator_pool.h"
#include "tac_dx12_gpu_upload_allocator.h"

#include "tac-win32/dx/dx12/tac_dx12_helper.h"
#include "tac-win32/dx/dx12/buffer/tac_dx12_frame_buf_mgr.h"

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/containers/tac_fixed_vector.h"

#include <WinPixEventRuntime/pix3.h>

#if !TAC_DELETE_ME()
#include "tac-std-lib/os/tac_os.h"
#endif


namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  // DX12Context
#if 0

  DX12Context::DX12Context( DX12CommandAllocatorPool* pool,
                            DX12ContextManager* mgr,
                            DX12CommandQueue* q,
                            Errors* e )
  {
    mCommandAllocatorPool = mCommandAllocatorPool;
    mContextManager = mgr;
    mCommandQueue = mCommandQueue;
    mParentScopeErrors = e;
  }

  DX12Context::DX12Context( DX12Context&& other ) noexcept
  {
    MoveFrom( ( DX12Context&& )other );
  }

  void DX12Context::MoveFrom( DX12Context&& other ) noexcept
  {
    mCommandList = other.mCommandList;
    mCommandAllocator = other.mCommandAllocator;
    mGPUUploadAllocator = other.mGPUUploadAllocator;
    mExecuted = other.mExecuted;
    mSynchronous = other.mSynchronous;
    mCommandAllocatorPool = other.mCommandAllocatorPool;
    mContextManager = other.mContextManager;
    mCommandQueue = other.mCommandQueue;
  }

  void DX12Context::operator = ( DX12Context&& other ) noexcept
  {
    MoveFrom( ( DX12Context&& )other );
  }


#endif

  void DX12Context::Execute( Errors& errors )
  {
    TAC_ASSERT( !mExecuted );

    ID3D12GraphicsCommandList* commandList = GetCommandList();
    if( !commandList )
      return; // This context has been (&&) moved

    for( int i = 0; i < mEventCount; ++i )
      PIXEndEvent( commandList );
    mEventCount = 0;

    // Indicates that recording to the command list has finished.
    TAC_DX12_CALL( commandList->Close() );

    const FenceSignal fenceSignal = TAC_CALL(
      mCommandQueue->ExecuteCommandList( commandList, errors ) );

    if( mSynchronous )
    {
      mCommandQueue->WaitForFence( fenceSignal, errors );
      TAC_ASSERT( !errors );
    }

    mCommandAllocatorPool->Retire( mCommandAllocator, fenceSignal );
    mCommandAllocator = {};
    mGPUUploadAllocator.FreeAll( fenceSignal );

    mExecuted = true;
  }

  ID3D12GraphicsCommandList* DX12Context::GetCommandList() { return mCommandList.Get(); }
  ID3D12CommandAllocator* DX12Context::GetCommandAllocator() { return mCommandAllocator.Get(); }

  void DX12Context::Reset( Errors& errors )
  {
    mSynchronous = false;
    mExecuted = false;
    //mGPUUploadAllocator; // <-- should be clear

    TAC_ASSERT( !mCommandAllocator );

    mCommandAllocator =
      TAC_CALL( mCommandAllocatorPool->GetAllocator( errors ) );

    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    //
    // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandallocator-reset
    // ID3D12CommandAllocator::Reset
    //   Indicates to re-use the memory that is associated with the command allocator.
    //   From this call to Reset, the runtime and driver determine that the GPU is no longer
    //   executing any command lists that have recorded commands with the command allocator.
    ID3D12CommandAllocator* dxCommandAllocator = GetCommandAllocator();
    TAC_DX12_CALL( mCommandAllocator->Reset() );


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
    ID3D12GraphicsCommandList* dxCommandList = GetCommandList();
    TAC_DX12_CALL( dxCommandList->Reset( dxCommandAllocator, nullptr ) );

  }

  void DX12Context::SetName( StringView name )
  {
    DX12SetName( mCommandAllocator, name );
    DX12SetName( mCommandList, name );
  }

  void DX12Context::SetSynchronous()
  {
    mSynchronous = true;
  }

  void DX12Context::SetViewport( v2i size )
  {
    ID3D12GraphicsCommandList* cmd = GetCommandList();

    D3D12_VIEWPORT vp
    {
      .TopLeftX = 0,
      .TopLeftY = 0,
      .Width = ( FLOAT )size.x,
      .Height = ( FLOAT )size.y,
      .MinDepth = 0,
      .MaxDepth = 1,
    };
    cmd->RSSetViewports( 1, &vp );
  }

  void DX12Context::SetScissor( v2i size )
  {
    ID3D12GraphicsCommandList* cmd = GetCommandList();
    D3D12_RECT rect
    {
      .right = ( LONG )size.x,
      .bottom = ( LONG )size.y,
    };
    cmd->RSSetScissorRects( 1, &rect );
  }

  void DX12Context::DebugEventBegin( StringView str )
  {
    ID3D12GraphicsCommandList* commandList = GetCommandList();
    PIXBeginEvent( commandList, PIX_COLOR_DEFAULT, str );
    mEventCount++;
  }

  void DX12Context::DebugEventEnd()
  {
    TAC_ASSERT( mEventCount > 0 );
    ID3D12GraphicsCommandList* commandList = GetCommandList();
    PIXEndEvent( commandList );
    mEventCount--;
  }

  void DX12Context::DebugMarker( StringView str )
  {
    ID3D12GraphicsCommandList* commandList = GetCommandList();
    PIXSetMarker( commandList, PIX_COLOR_DEFAULT, str );
  }

  void DX12Context::SetRenderTargets( Targets targets )
  {
    ID3D12GraphicsCommandList* commandList = GetCommandList();

    FixedVector< D3D12_CPU_DESCRIPTOR_HANDLE, 10 > rtDescs;

    OS::OSDebugBreak();
#if TAC_TEMPORARILY_DISABLED()


    DX12SwapChain* frameBuf = mFrameBufferMgr->FindSwapChain( h );
    const UINT bbIdx = frameBuf->mSwapChain->GetCurrentBackBufferIndex();
    DX12SwapChainImage& swapChainImage = frameBuf->mSwapChainImages[ bbIdx ];

    D3D12_CPU_DESCRIPTOR_HANDLE descHandle = swapChainImage.mRTV.GetCPUHandle();
    rtDescs.push_back( descHandle );

    BOOL RTsSingleHandleToDescriptorRange = false;

    D3D12_RESOURCE_BARRIER barrier
    {
      .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
      .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
      .Transition = D3D12_RESOURCE_TRANSITION_BARRIER
      {
        .pResource = swapChainImage.mResource.Get(),
        .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        .StateBefore = swapChainImage.mState,
        .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
      },
    };

    const Array barriers = { barrier };
    commandList->ResourceBarrier( ( UINT )barriers.size(), barriers.data() );

    // if null, no depth stencil is bound
    const D3D12_CPU_DESCRIPTOR_HANDLE* pDepthStencilDescriptor{};
    commandList->OMSetRenderTargets( ( UINT )rtDescs.size(),
                                     rtDescs.data(),
                                     RTsSingleHandleToDescriptorRange,
                                     pDepthStencilDescriptor );

    swapChainImage.mState = D3D12_RESOURCE_STATE_RENDER_TARGET;

#endif
  }


  void DX12Context::Retire()
  {
    mContextManager->RetireContext( this );
  }

  // -----------------------------------------------------------------------------------------------

  // DX12ContextManager

  void DX12ContextManager::RetireContext( DX12Context* context )
  {
    mAvailableContexts.push_back( context );
  }


  void DX12ContextManager::Init( DX12CommandAllocatorPool* commandAllocatorPool,
                                 DX12CommandQueue* commandQueue,
                                 DX12UploadPageMgr* uploadPageManager,
                                 DX12SwapChainMgr* frameBufferMgr,
                                 ID3D12Device* device )
  {
    mCommandAllocatorPool = commandAllocatorPool;
    mCommandQueue = commandQueue;
    mUploadPageManager = uploadPageManager;
    mFrameBufferMgr = frameBufferMgr;
    device->QueryInterface( mDevice.iid(), mDevice.ppv() );
    TAC_ASSERT( mDevice );
  }

  DX12Context* DX12ContextManager::GetContext( Errors& errors )
  {
    DX12Context* dx12Context{};

    if( mAvailableContexts.empty() )
    {
      dx12Context = TAC_NEW DX12Context;
      dx12Context->mCommandList = TAC_CALL_RET( {}, CreateCommandList( errors ) );
      dx12Context->mGPUUploadAllocator.Init( mUploadPageManager );
      dx12Context->mCommandAllocatorPool = mCommandAllocatorPool;
      dx12Context->mContextManager = this;
      dx12Context->mCommandQueue = mCommandQueue;
      dx12Context->mFrameBufferMgr = mFrameBufferMgr;
      dx12Context->mCommandList = TAC_CALL_RET( {}, CreateCommandList( errors ) );
    }
    else
    {
      dx12Context = mAvailableContexts.back();
      mAvailableContexts.pop_back();
    }

    TAC_CALL_RET( {}, dx12Context->Reset( errors ) );

    return dx12Context;
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


} // namespace Tac::Render
