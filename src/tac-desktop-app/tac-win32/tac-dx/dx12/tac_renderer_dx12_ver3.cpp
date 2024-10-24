#include "tac_renderer_dx12_ver3.h" // self-inc

#include "tac-dx/dx12/tac_dx12_transition_helper.h"
#include "tac-dx/dx12/tac_dx12_helper.h" // TAC_DX12_CALL
#include "tac-dx/dxgi/tac_dxgi.h" // DXGICreateSwapChain

#pragma comment( lib, "d3d12.lib" ) // D3D12...

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  DX12DescriptorHeap& DX12Renderer::GetCpuHeap_RTV()
  {
    return mCpuDescriptorHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_RTV ];
  }
  DX12DescriptorHeap& DX12Renderer::GetCpuHeap_DSV()
  {
    return mCpuDescriptorHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_DSV ];
  }
  DX12DescriptorHeap& DX12Renderer::GetCpuHeap_CBV_SRV_UAV()
  {
    return mCpuDescriptorHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ];
  }
  DX12DescriptorHeap& DX12Renderer::GetCpuHeap_Sampler()
  {
    return mCpuDescriptorHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ];
  }

  DX12DescriptorHeap& DX12Renderer::GetGPUHeap_CBV_SRV_UAV()
  {
    return mGpuDescriptorHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ];
  }
  DX12DescriptorHeap& DX12Renderer::GetGPUHeap_Sampler()
  {
    return mGpuDescriptorHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ];
  }

  DX12Renderer DX12Renderer::sRenderer;

  // -----------------------------------------------------------------------------------------------

  void DeletionQueue::Push( SwapChainHandle h)
  {
    const DeletionQueue::Entry entry
    {
      .mSwapChainHandle { h },
      .mFrame        { DX12Renderer::sRenderer.mRenderFrame },
    };
    mEntries.push( entry );
  }
  void DeletionQueue::Push( PipelineHandle h)
  {
    const DeletionQueue::Entry entry
    {
      .mPipelineHandle { h },
      .mFrame        { DX12Renderer::sRenderer.mRenderFrame },
    };
    mEntries.push( entry );
  }
  void DeletionQueue::Push( ProgramHandle h)
  {
    const DeletionQueue::Entry entry
    {
      .mProgramHandle { h },
      .mFrame        { DX12Renderer::sRenderer.mRenderFrame },
    };
    mEntries.push( entry );
  }
  void DeletionQueue::Push( BufferHandle h)
  {
    const DeletionQueue::Entry entry
    {
      .mBufferHandle { h },
      .mFrame        { DX12Renderer::sRenderer.mRenderFrame },
    };
    mEntries.push( entry );
  }
  void DeletionQueue::Push( TextureHandle h)
  {
    const DeletionQueue::Entry entry
    {
      .mTextureHandle { h },
      .mFrame        { DX12Renderer::sRenderer.mRenderFrame },
    };
    mEntries.push( entry );
  }
  void DeletionQueue::Push( SamplerHandle h)
  {
    const DeletionQueue::Entry entry
    {
      .mSamplerHandle { h },
      .mFrame        { DX12Renderer::sRenderer.mRenderFrame },
    };
    mEntries.push( entry );
  }

  void DeletionQueue::Update()
  {
    const int maxGPUFrameCount{ RenderApi::GetMaxGPUFrameCount() };
    DX12Renderer& renderer{ DX12Renderer::sRenderer };

    DX12SwapChainMgr& swapChainMgr{ renderer.mSwapChainMgr };
    DX12BufferMgr& bufMgr{ renderer.mBufMgr };
    DX12TextureMgr& texMgr{ renderer.mTexMgr };
    DX12ProgramMgr& programMgr{ renderer.mProgramMgr };
    DX12PipelineMgr& pipelineMgr{ renderer.mPipelineMgr };
    DX12SamplerMgr& samplerMgr{ renderer.mSamplerMgr };

    for( ;; )
    {
      if( mEntries.empty() )
        break;

      const Entry& entry{ mEntries.front() };
      if( entry.mFrame + maxGPUFrameCount < renderer.mRenderFrame )
        break;

      pipelineMgr.DestroyPipeline( entry.mPipelineHandle );
      swapChainMgr.DestroySwapChain( entry.mSwapChainHandle );
      bufMgr.DestroyBuffer( entry.mBufferHandle );
      texMgr.DestroyTexture( entry.mTextureHandle );
      programMgr.DestroyProgram( entry.mProgramHandle );
      samplerMgr.DestroySampler( entry.mSamplerHandle );

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

    TAC_CALL( InitDescriptorHeaps( errors ) );

    TAC_CALL( mProgramMgr.Init( errors ) );

    TAC_CALL( mCommandQueue.Create( mDevice, errors ) );

    TAC_CALL( mContextManager.Init( errors ) );
  }

  void DX12Renderer::InitDescriptorHeaps( Errors& errors )
  {
    ID3D12Device* device { mDevice };

    // CPU RTV
    {
      const D3D12_DESCRIPTOR_HEAP_DESC desc
      {
        .Type           { D3D12_DESCRIPTOR_HEAP_TYPE_RTV },
        .NumDescriptors { 25 },
        .Flags          { D3D12_DESCRIPTOR_HEAP_FLAG_NONE },
        .NodeMask       {},
      };

      const DX12DescriptorHeap::Params params
      {
        .mHeapDesc     { desc },
        .mDevice       { device },
        .mName         { "cpu rtv heap"},
      };

      TAC_CALL( GetCpuHeap_RTV().Init( params, errors ) );
    }

    // CPU DSV
    {
      const D3D12_DESCRIPTOR_HEAP_DESC desc
      {
        .Type           { D3D12_DESCRIPTOR_HEAP_TYPE_DSV },
        .NumDescriptors { 25 },
        .Flags          { D3D12_DESCRIPTOR_HEAP_FLAG_NONE },
        .NodeMask       {},
      };
      const DX12DescriptorHeap::Params params
      {
        .mHeapDesc     { desc },
        .mDevice       { device },
        .mName         { "cpu dsv heap" },
      };
      TAC_CALL( GetCpuHeap_DSV().Init( params, errors ) );
    }

    // CPU CBV SRV UAV
    {
      const D3D12_DESCRIPTOR_HEAP_DESC desc
      {
        .Type           { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV },
        .NumDescriptors { 100 },
        .Flags          { D3D12_DESCRIPTOR_HEAP_FLAG_NONE },
        .NodeMask       {},
      };
      const DX12DescriptorHeap::Params params
      {
        .mHeapDesc     { desc },
        .mDevice       { device },
        .mName         { "cpu cbv srv uav heap" },
      };
      TAC_CALL( GetCpuHeap_CBV_SRV_UAV().Init( params, errors ) );
    }

    // CPU Sampler
    {
      const D3D12_DESCRIPTOR_HEAP_DESC desc
      {
        .Type           { D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER },
        .NumDescriptors { 20 },
        .Flags          { D3D12_DESCRIPTOR_HEAP_FLAG_NONE },
        .NodeMask       {},
      };
      const DX12DescriptorHeap::Params params
      {
        .mHeapDesc     { desc },
        .mDevice       { device },
        .mName         { "cpu sampler heap" },
      };
      TAC_CALL( GetCpuHeap_Sampler().Init( params, errors ) );
    }

    // GPU Sampler
    {
      const D3D12_DESCRIPTOR_HEAP_DESC desc
      {
        .Type           { D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER },
        .NumDescriptors { 1000 },
        .Flags          { D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE },
        .NodeMask       {},
      };
      const DX12DescriptorHeap::Params params
      {
        .mHeapDesc     { desc },
        .mDevice       { device },
        .mName         { "gpu sampler heap" },
        .mCommandQueue { &mCommandQueue },
      };
      TAC_CALL( GetGPUHeap_Sampler().Init( params, errors ) );
    }

    // GPU CBV SRV UAV
    {
      const D3D12_DESCRIPTOR_HEAP_DESC desc
      {
        .Type           { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV },
        .NumDescriptors { 1000 },
        .Flags          { D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE },
        .NodeMask       {},
      };
      const DX12DescriptorHeap::Params params
      {
        .mHeapDesc     { desc },
        .mDevice       { device },
        .mName         { "gpu cbv srv uav heap" },
        .mCommandQueue { &mCommandQueue },
      };
      TAC_CALL( GetGPUHeap_CBV_SRV_UAV().Init( params, errors ) );
    }
  }

  // -----------------------------------------------------------------------------------------------

  void              DX12Device::Init( Errors& errors )
  {
    DX12Renderer::sRenderer.Init( errors );
  }

  void              DX12Device::Update( Errors& errors )
  {
    DX12Renderer::sRenderer.mProgramMgr.HotReload( errors );
    DX12Renderer::sRenderer.mRenderFrame++;
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

  TextureHandle     DX12Device::CreateTexture( CreateTextureParams params, Errors& errors )
  {
    return  DX12Renderer::sRenderer.mTexMgr.CreateTexture( params, errors );
  }

  void              DX12Device::DestroyTexture( TextureHandle h )
  {
    DX12Renderer::sRenderer.mDeletionQueue.Push( h );
  }

} // namespace Tac::Render

