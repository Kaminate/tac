#include "tac_renderer_dx12_ver3.h" // self-inc

#include "tac-dx/dx12/tac_dx12_transition_helper.h"
#include "tac-dx/dx12/tac_dx12_helper.h" // TAC_DX12_CALL
#include "tac-dx/dxgi/tac_dxgi.h" // DXGICreateSwapChain

#pragma comment( lib, "d3d12.lib" ) // D3D12...

#include <vector>

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  DX12Renderer DX12Renderer::sRenderer;

  // -----------------------------------------------------------------------------------------------

  void DeletionQueue::Push( ResourceHandle h )
  {
    if( !h.IsValid() )
      return;

    const DeletionQueue::Entry entry
    {
      .mResourceHandle { h },
      .mFrame          { DX12Renderer::sRenderer.mRenderFrame },
    };
    mEntries.push( entry );
  }

  void DeletionQueue::Update()
  {
    const int maxGPUFrameCount{ RenderApi::GetMaxGPUFrameCount() };
    DX12Renderer& renderer{ DX12Renderer::sRenderer };

    DX12SwapChainMgr& swapChainMgr { renderer.mSwapChainMgr };
    DX12BufferMgr&    bufMgr       { renderer.mBufMgr };
    DX12TextureMgr&   texMgr       { renderer.mTexMgr };
    DX12ProgramMgr&   programMgr   { renderer.mProgramMgr };
    DX12PipelineMgr&  pipelineMgr  { renderer.mPipelineMgr };
    DX12SamplerMgr&   samplerMgr   { renderer.mSamplerMgr };

    for( ;; )
    {
      if( mEntries.empty() )
        break;

      const Entry& entry{ mEntries.front() };
      if( entry.mFrame + maxGPUFrameCount < renderer.mRenderFrame )
        break;

      const HandleType handleType{ entry.mResourceHandle.GetHandleType() };

      switch( handleType )
      {
      case HandleType::kPipeline:
        pipelineMgr.DestroyPipeline( entry.mResourceHandle );
        break;
      case HandleType::kSwapChain:
        swapChainMgr.DestroySwapChain( entry.mResourceHandle );
        break;
      case HandleType::kBuffer:
        bufMgr.DestroyBuffer( entry.mResourceHandle );
        break;
      case HandleType::kTexture:
        texMgr.DestroyTexture( entry.mResourceHandle );
        break;
      case HandleType::kProgram:
        programMgr.DestroyProgram( entry.mResourceHandle );
        break;
      case HandleType::kSampler:
        samplerMgr.DestroySampler( entry.mResourceHandle );
        break;
      default: TAC_ASSERT_INVALID_CASE( handleType ); break;
      }

      mEntries.pop();
    }
  }

  // -----------------------------------------------------------------------------------------------

  void DX12Renderer::Init( Errors& errors )
  {
    TAC_CALL( DXGIInit( errors ) );

    TAC_CALL( mDebugLayer.Init( errors ) );

    TAC_CALL( mDeviceInitializer.Init( mDebugLayer, errors ) );

    mDevice = mDeviceInitializer.mDevice.Get();

    TAC_CALL( mInfoQueue.Init( mDebugLayer, mDevice, errors ) );

    TAC_CALL( mDescriptorHeapMgr.Init( errors ) );

    TAC_CALL( mProgramMgr.Init( errors ) );

    TAC_CALL( mCommandQueue.Create( mDevice, errors ) );

    TAC_CALL( mContextManager.Init( errors ) );
  }

  void DX12Renderer::Update( Errors& errors )
  {
    mDeletionQueue.Update();
    mProgramMgr.HotReload( errors );
    mRenderFrame++;
  }

  // -----------------------------------------------------------------------------------------------

  void              DX12Device::Init( Errors& errors )
  {
    DX12Renderer::sRenderer.Init( errors );
  }

  void              DX12Device::Update( Errors& errors )
  {
    TAC_CALL( DX12Renderer::sRenderer.Update( errors ) );
  }

  IDevice::Info     DX12Device::GetInfo() const
  {
    const NDCAttribs ndcAttribs
    {
      .mMinZ {},
      .mMaxZ { 1 },
    };

    const ProgramAttribs programAttribs
    {
      .mDir { "assets/hlsl/" },
      .mExt { ".hlsl" },
    };

    return Info
    {
      .mNDCAttribs     { ndcAttribs },
      .mProgramAttribs { programAttribs },
    };
  }

  PipelineHandle    DX12Device::CreatePipeline( PipelineParams params,
                                                Errors& errors )
  {
    return DX12Renderer::sRenderer.mPipelineMgr.CreatePipeline( params, errors );
  }

  IShaderVar*       DX12Device::GetShaderVariable( PipelineHandle h, StringView sv )
  {
    DX12Pipeline* pipeline { DX12Renderer::sRenderer.mPipelineMgr.FindPipeline( h ) };
    for( DX12Pipeline::Variable& var : pipeline->mShaderVariables )
      if( var.GetName() == sv )
        return &var;

    TAC_ASSERT_INVALID_CODE_PATH;
    return nullptr;
  }

  void              DX12Device::DestroyPipeline( PipelineHandle h )
  {
    DX12Renderer::sRenderer.mDeletionQueue.Push( h );
  }

  ProgramHandle     DX12Device::CreateProgram( ProgramParams params, Errors& errors )
  {
    return DX12Renderer::sRenderer.mProgramMgr.CreateProgram( params, errors );
  }

  String            DX12Device::GetProgramBindings_TEST( ProgramHandle h )
  {
    return DX12Renderer::sRenderer.mProgramMgr.GetProgramBindings_TEST( h );
  }

  void              DX12Device::DestroyProgram( ProgramHandle h )
  {
    DX12Renderer::sRenderer.mDeletionQueue.Push( h );
  }

  SamplerHandle     DX12Device::CreateSampler( CreateSamplerParams params )
  {
    return DX12Renderer::sRenderer.mSamplerMgr.CreateSampler( params );
  }

  void              DX12Device::DestroySampler( SamplerHandle h )
  {
    DX12Renderer::sRenderer.mDeletionQueue.Push( h );
  }

  SwapChainHandle   DX12Device::CreateSwapChain( SwapChainParams params, Errors& errors )
  {
    return DX12Renderer::sRenderer.mSwapChainMgr.CreateSwapChain( params, errors );
  }

  void              DX12Device::ResizeSwapChain( SwapChainHandle h, v2i size, Errors& errors )
  {
    DX12Renderer::sRenderer.mSwapChainMgr.ResizeSwapChain( h, size, errors );
  }

  SwapChainParams   DX12Device::GetSwapChainParams( SwapChainHandle h )
  {
    return DX12Renderer::sRenderer.mSwapChainMgr.GetSwapChainParams( h );
  }

  void              DX12Device::DestroySwapChain( SwapChainHandle h )
  {
    DX12Renderer::sRenderer.mDeletionQueue.Push( h );
  }

  TextureHandle     DX12Device::GetSwapChainCurrentColor( SwapChainHandle h)
  {
    return DX12Renderer::sRenderer.mSwapChainMgr.GetSwapChainCurrentColor( h );
  }

  TextureHandle     DX12Device::GetSwapChainDepth( SwapChainHandle h )
  {
    return DX12Renderer::sRenderer.mSwapChainMgr.GetSwapChainDepth( h );
  }

  void              DX12Device::Present( SwapChainHandle h, Errors& errors )
  {
    DX12SwapChain* swapChain{ DX12Renderer::sRenderer.mSwapChainMgr.FindSwapChain( h ) };
    TAC_ASSERT( swapChain );

    IDXGISwapChain4* swapChain4 { swapChain->mDXGISwapChain.GetIDXGISwapChain() };

    TextureHandle textureHandle{ DX12Renderer::sRenderer.mSwapChainMgr.GetSwapChainCurrentColor( h ) };

    DX12Texture* texture{ DX12Renderer::sRenderer.mTexMgr.FindTexture( textureHandle ) };

    const DX12TransitionHelper::Params transitionParams
    {
      .mResource    { &texture->mResource },
      .mStateAfter  { D3D12_RESOURCE_STATE_PRESENT },
    };

    DX12TransitionHelper transitionHelper;
    transitionHelper.Append( transitionParams );
    if( !transitionHelper.empty() )
    {
      IContext::Scope scope{ CreateRenderContext( errors ) };
      DX12Context* context{ ( DX12Context* )scope.GetContext() };
      ID3D12GraphicsCommandList* commandList{ context->GetCommandList() };
      transitionHelper.ResourceBarrier( commandList );
      TAC_CALL( context->Execute( errors ) );
    }

    const DXGI_PRESENT_PARAMETERS params{};

    // For the flip model (DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL), values are:
    //   0   - Cancel the remaining time on the previously presented frame
    //         and discard this frame if a newer frame is queued.
    //   1-4 - Synchronize presentation for at least n vertical blanks.
    const UINT SyncInterval { 1 };
    const UINT PresentFlags {};

    // I think this technically adds a frame onto the present queue
    TAC_DX12_CALL( swapChain4->Present1( SyncInterval, PresentFlags, &params ) );
  }

  BufferHandle      DX12Device::CreateBuffer( CreateBufferParams params,
                                              Errors& errors )
  {
    return DX12Renderer::sRenderer.mBufMgr.CreateBuffer( params, errors );
  }

  void              DX12Device::DestroyBuffer( BufferHandle h )
  {
    DX12Renderer::sRenderer.mDeletionQueue.Push( h );
  }

  IContext::Scope   DX12Device::CreateRenderContext( Errors& errors )
  {
    DX12Context* context{ DX12Renderer::sRenderer.mContextManager.GetContext( errors ) };
    return IContext::Scope( context );
  }

  IBindlessArray*   DX12Device::CreateBindlessArray( IBindlessArray::Params params )
  {
    return TAC_NEW BindlessArray(params);
  }

  TextureHandle     DX12Device::CreateTexture( CreateTextureParams params, Errors& errors )
  {
    return  DX12Renderer::sRenderer.mTexMgr.CreateTexture( params, errors );
  }

  void              DX12Device::DestroyTexture( TextureHandle h )
  {
    DX12Renderer::sRenderer.mDeletionQueue.Push( h );
  }

} // namespace Tac::Render

