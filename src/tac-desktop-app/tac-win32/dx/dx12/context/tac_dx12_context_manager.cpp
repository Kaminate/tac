#include "tac_dx12_context_manager.h" // self-inc

#include "tac-win32/dx/dx12/tac_dx12_command_allocator_pool.h"
#include "tac-win32/dx/dx12/tac_dx12_gpu_upload_allocator.h"
#include "tac-win32/dx/dx12/texture/tac_dx12_texture_mgr.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h"
#include "tac-win32/dx/dx12/tac_renderer_dx12_ver3.h"
#include "tac-win32/dx/dx12/buffer/tac_dx12_frame_buf_mgr.h"

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/containers/tac_fixed_vector.h"

#if !TAC_DELETE_ME()
#include "tac-std-lib/os/tac_os.h"
#endif


namespace Tac::Render
{
  void DX12ContextManager::RetireContext( DX12Context* context )
  {
    mAvailableContexts.push_back( context );
  }

  void DX12ContextManager::Init( Params params )
  {
    mCommandAllocatorPool = params.mCommandAllocatorPool;
    mCommandQueue = params.mCommandQueue;
    mUploadPageManager = params.mUploadPageManager;
    mSwapChainMgr = params.mSwapChainMgr;
    mTextureMgr = params.mTextureMgr;
    mBufferMgr = params.mBufferMgr;
    mPipelineMgr = params.mPipelineMgr;
    params.mDevice->QueryInterface( mDevice.iid(), mDevice.ppv() );
    TAC_ASSERT( mCommandAllocatorPool &&
                mCommandQueue &&
                mUploadPageManager &&
                mSwapChainMgr &&
                mTextureMgr &&
                mBufferMgr &&
                mPipelineMgr &&
                mDevice );
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
      dx12Context->mFrameBufferMgr = mSwapChainMgr;
      dx12Context->mTextureMgr = mTextureMgr;
      dx12Context->mBufferMgr = mBufferMgr;
      dx12Context->mPipelineMgr = mPipelineMgr;
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

    PCom< ID3D12GraphicsCommandList > graphicsList{
      commandList.QueryInterface<ID3D12GraphicsCommandList>() };

    TAC_ASSERT( graphicsList );
    DX12SetName( graphicsList, "My Command List" );
    return graphicsList;
  }


} // namespace Tac::Render
