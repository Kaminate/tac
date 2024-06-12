#include "tac_renderer_dx12_ver3.h" // self-inc
#include "tac_dx12_transition_helper.h"

//#include "tac-rhi/render3/tac_render_api.h"

#if !TAC_DELETE_ME()
#include "tac-std-lib/os/tac_os.h" // OS::DebugBreak
#endif
//#include "tac-rhi/render/tac_render.h"
//#include "tac-rhi/render/tac_render.h"
//#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-dx/dx12/tac_dx12_helper.h" // TAC_DX12_CALL
#include "tac-dx/dxgi/tac_dxgi.h" // DXGICreateSwapChain

#pragma comment( lib, "d3d12.lib" ) // D3D12...

namespace Tac::Render
{

  static DX12Renderer sRenderer;

  // -----------------------------------------------------------------------------------------------

  void DX12Renderer::Init( Errors& errors )
  {
    TAC_CALL( DXGIInit( errors ) );

    TAC_CALL( mDebugLayer.Init( errors ) );

    TAC_CALL( mDeviceInitializer.Init( mDebugLayer, errors ) );

    mDevice = mDeviceInitializer.mDevice.Get();

    TAC_CALL( mInfoQueue.Init( mDebugLayer, mDevice, errors ) );

    TAC_CALL( InitDescriptorHeaps( errors ) );

    TAC_CALL( mProgramMgr.Init( mDevice, errors ) );

    TAC_CALL( mPipelineMgr.Init( mDevice, &mProgramMgr ) );

    mTexMgr.Init( {
      .mDevice                       { mDevice },
      .mCpuDescriptorHeapRTV         { &mCpuDescriptorHeapRTV },
      .mCpuDescriptorHeapDSV         { &mCpuDescriptorHeapDSV },
      .mCpuDescriptorHeapCBV_SRV_UAV { &mCpuDescriptorHeapCBV_SRV_UAV },
      .mContextManager               { &mContextManager },
                  } );

    mSwapChainMgr.Init( {
      .mTextureManager   { &mTexMgr },
      .mCommandQueue     { &mCommandQueue },
                        } );

    mBufMgr.Init( {
      .mDevice                       { mDevice },
      .mCpuDescriptorHeapCBV_SRV_UAV { &mCpuDescriptorHeapCBV_SRV_UAV },
      .mContextManager               { &mContextManager },
                  } );

    //const int maxGPUFrameCount = RenderApi::GetMaxGPUFrameCount();
    /*
    const int maxGPUFrameCount = Render::GetMaxGPUFrameCount();
    TAC_ASSERT( maxGPUFrameCount );
    mFenceValues.resize( maxGPUFrameCount );

    TAC_CALL( DXGIInit( errors ) );

    TAC_CALL( debugLayer.Init( errors ) );

    TAC_CALL( mDevice.Init( debugLayer, errors ) );
    ID3D12Device* device = mDevice.GetID3D12Device();

    TAC_CALL( infoQueue.Init( debugLayer, device, errors ) );

    */
    TAC_CALL( mCommandQueue.Create( mDevice, errors ) );
    mCommandAllocatorPool.Init( mDevice, &mCommandQueue );

    mContextManager.Init( {
      .mCommandAllocatorPool         { &mCommandAllocatorPool },
      .mCommandQueue                 { &mCommandQueue },
      .mUploadPageManager            { &mUploadPageManager },
      .mSwapChainMgr                 { &mSwapChainMgr },
      .mTextureMgr                   { &mTexMgr },
      .mBufferMgr                    { &mBufMgr },
      .mPipelineMgr                  { &mPipelineMgr },
      .mSamplerMgr                   { &mSamplerMgr },
      .mDevice                       { mDevice },
      .mGpuDescriptorHeapCBV_SRV_UAV { &mGpuDescriptorHeapCBV_SRV_UAV },
      .mGpuDescriptorHeapSampler     { &mGpuDescriptorHeapSampler },
                          } );
      
    mUploadPageManager.Init( mDevice, &mCommandQueue );
    /*

    TAC_CALL( mSRVDescriptorHeap.InitSRV( 100, device, errors ) );
    TAC_CALL( mSamplerDescriptorHeap.InitSampler( 100, device, errors ) );



    mSamplers.Init( device, &mSamplerDescriptorHeap );
    */

    mSamplerMgr.Init( {
      .mDevice                   { mDevice },
      .mCpuDescriptorHeapSampler { &mCpuDescriptorHeapSampler },
                      });
  }

  void DX12Renderer::InitDescriptorHeaps( Errors& errors )
  {
    ID3D12Device* device = mDevice;

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

      TAC_CALL( mCpuDescriptorHeapRTV.Init( params, errors ) );
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
      TAC_CALL( mCpuDescriptorHeapDSV.Init( params, errors ) );
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
      TAC_CALL( mCpuDescriptorHeapCBV_SRV_UAV.Init( params, errors ) );
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
      TAC_CALL( mCpuDescriptorHeapSampler.Init( params, errors ) );
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
      TAC_CALL( mGpuDescriptorHeapSampler.Init( params, errors ) );
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
      TAC_CALL( mGpuDescriptorHeapCBV_SRV_UAV.Init( params, errors ) );
    }
  }

  // -----------------------------------------------------------------------------------------------

  void              DX12Device::Init( Errors& errors )
  {
    sRenderer.Init( errors );
  }

  IDevice::Info     DX12Device::GetInfo() const
  {
    const NDCAttribs ndcAttribs
    {
      .mMinZ{ 0 },
      .mMaxZ{ 1 },
    };

    return Info
    {
      .mNDCAttribs{ndcAttribs},
    };
  }

  PipelineHandle    DX12Device::CreatePipeline( PipelineParams params,
                                                Errors& errors )
  {
    const PipelineHandle h{ AllocPipelineHandle() };
    sRenderer.mPipelineMgr.CreatePipeline( h, params, errors );
    return h;
  }

  IShaderVar*       DX12Device::GetShaderVariable( PipelineHandle h, StringView sv )
  {
    DX12Pipeline* pipeline { sRenderer.mPipelineMgr.FindPipeline( h ) };
    for( DX12Pipeline::Variable& var : pipeline->mShaderVariables )
      if( var.GetName() == sv )
        return &var;

    TAC_ASSERT_INVALID_CODE_PATH;

    return nullptr;
  }

  void              DX12Device::DestroyPipeline( PipelineHandle h )
  {
    if( h.IsValid() )
    {
      FreeHandle( h );
      sRenderer.mPipelineMgr.DestroyPipeline( h );
    }
  }

  ProgramHandle     DX12Device::CreateProgram( ProgramParams params, Errors& errors )
  {
    return sRenderer.mProgramMgr.CreateProgram( params, errors );
  }

  void              DX12Device::DestroyProgram( ProgramHandle h )
  {
    sRenderer.mProgramMgr.DestroyProgram( h );
  }

  SamplerHandle     DX12Device::CreateSampler( CreateSamplerParams params )
  {
    return sRenderer.mSamplerMgr.CreateSampler( params );
  }

  void              DX12Device::DestroySampler( SamplerHandle h )
  {
    sRenderer.mSamplerMgr.DestroySampler( h );
  }

  SwapChainHandle   DX12Device::CreateSwapChain( SwapChainParams params, Errors& errors )
  {
    return sRenderer.mSwapChainMgr.CreateSwapChain( params, errors );
  }

  void              DX12Device::ResizeSwapChain( SwapChainHandle h, v2i size, Errors& errors )
  {
    sRenderer.mSwapChainMgr.ResizeSwapChain( h, size, errors );
  }

  SwapChainParams   DX12Device::GetSwapChainParams( SwapChainHandle h )
  {
    return sRenderer.mSwapChainMgr.GetSwapChainParams( h );
  }

  void              DX12Device::DestroySwapChain( SwapChainHandle h )
  {
    return sRenderer.mSwapChainMgr.DestroySwapChain( h );
  }

  TextureHandle     DX12Device::GetSwapChainCurrentColor( SwapChainHandle h)
  {
    return sRenderer.mSwapChainMgr.GetSwapChainCurrentColor( h );
  }

  TextureHandle     DX12Device::GetSwapChainDepth( SwapChainHandle h )
  {
    return sRenderer.mSwapChainMgr.GetSwapChainDepth( h );
  }

  void              DX12Device::Present( SwapChainHandle h, Errors& errors )
  {
    DX12SwapChain* swapChain{ sRenderer.mSwapChainMgr.FindSwapChain( h ) };
    TAC_ASSERT( swapChain );

    IDXGISwapChain4* swapChain4 { swapChain->mDXGISwapChain.GetIDXGISwapChain() };

    TextureHandle textureHandle{ sRenderer.mSwapChainMgr.GetSwapChainCurrentColor( h ) };

    DX12Texture* texture{ sRenderer.mTexMgr.FindTexture( textureHandle ) };

    const DX12TransitionHelper::Params transitionParams
    {
      .mResource    { texture->mResource.Get() },
      .mStateBefore { &texture->mState },
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
    return sRenderer.mBufMgr.CreateBuffer( params, errors );
  }

  void              DX12Device::DestroyBuffer( BufferHandle h )
  {
    sRenderer.mBufMgr.DestroyBuffer( h );
  }

  IContext::Scope   DX12Device::CreateRenderContext( Errors& errors )
  {
    DX12Context* context{ sRenderer.mContextManager.GetContext( errors ) };
    return IContext::Scope( context );
  }

  TextureHandle     DX12Device::CreateTexture( CreateTextureParams params, Errors& errors )
  {
    return  sRenderer.mTexMgr.CreateTexture( params, errors );
  }

  void              DX12Device::DestroyTexture( TextureHandle h )
  {
    sRenderer.mTexMgr.DestroyTexture( h );
  }

} // namespace Tac::Render

