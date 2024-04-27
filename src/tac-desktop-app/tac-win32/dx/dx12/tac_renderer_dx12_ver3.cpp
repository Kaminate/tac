#include "tac_renderer_dx12_ver3.h" // self-inc

//#include "tac-rhi/render3/tac_render_api.h"

#if !TAC_DELETE_ME()
#include "tac-std-lib/os/tac_os.h" // OS::DebugBreak
#endif
//#include "tac-rhi/render/tac_render.h"
//#include "tac-rhi/render/tac_render.h"
//#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h" // TAC_DX12_CALL
#include "tac-win32/dx/dxgi/tac_dxgi.h" // DXGICreateSwapChain

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

    mDevice = mDeviceInitializer.m_device.Get();

    TAC_CALL( mInfoQueue.Init( mDebugLayer, mDevice, errors ) );

    TAC_CALL( InitDescriptorHeaps( errors ) );

    TAC_CALL( mProgramMgr.Init( mDevice, errors ) );

    TAC_CALL( mPipelineMgr.Init( mDevice, &mProgramMgr ) );

    mTexMgr.Init( {
                    .mDevice               { mDevice },
                    .mCpuDescriptorHeapRTV { &mCpuDescriptorHeapRTV },
                  } );

    mSwapChainMgr.Init( {
                          .mTextureManager { &mTexMgr },
                          .mCommandQueue   { &mCommandQueue },
                        } );

    mBufMgr.Init( mDevice );

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
    mContextManager.Init( &mCommandAllocatorPool,
                          &mCommandQueue,
                          &mUploadPageManager,
                          &mSwapChainMgr,
                          mDevice,
                          this );
    mUploadPageManager.Init( mDevice, &mCommandQueue );
    /*

    TAC_CALL( mSRVDescriptorHeap.InitSRV( 100, device, errors ) );
    TAC_CALL( mSamplerDescriptorHeap.InitSampler( 100, device, errors ) );



    mSamplers.Init( device, &mSamplerDescriptorHeap );
    */
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
        .NodeMask       { 0 },
      };
      TAC_CALL(mCpuDescriptorHeapRTV.Init( desc, device, errors ));
    }

    // CPU DSV
    {
      const D3D12_DESCRIPTOR_HEAP_DESC desc
      {
        .Type           { D3D12_DESCRIPTOR_HEAP_TYPE_DSV },
        .NumDescriptors { 25 },
        .Flags          { D3D12_DESCRIPTOR_HEAP_FLAG_NONE },
        .NodeMask       { 0 },
      };
      TAC_CALL(mCpuDescriptorHeapDSV.Init( desc, device, errors ));
    }

    // CPU CBV SRV UAV
    {
      const D3D12_DESCRIPTOR_HEAP_DESC desc
      {
        .Type           { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV },
        .NumDescriptors { 100 },
        .Flags          { D3D12_DESCRIPTOR_HEAP_FLAG_NONE },
        .NodeMask       { 0 },
      };
      TAC_CALL(mCpuDescriptorHeapCBV_SRV_UAV.Init( desc, device, errors ));
    }

    // CPU Sampler
    {
      const D3D12_DESCRIPTOR_HEAP_DESC desc
      {
        .Type           { D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER },
        .NumDescriptors { 10 },
        .Flags          { D3D12_DESCRIPTOR_HEAP_FLAG_NONE },
        .NodeMask       { 0 },
      };
      TAC_CALL(mCpuDescriptorHeapSampler.Init( desc, device, errors ));
    }


    // GPU Sampler
    {
      const D3D12_DESCRIPTOR_HEAP_DESC desc
      {
        .Type           { D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER },
        .NumDescriptors { 10 },
        .Flags          { D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE },
        .NodeMask       { 0 },
      };
      TAC_CALL(mGpuDescriptorHeapSampler.Init( desc, device, errors ));
    }

    // GPU CBV SRV UAV
    {
      const D3D12_DESCRIPTOR_HEAP_DESC desc
      {
        .Type           { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV },
        .NumDescriptors { 100 },
        .Flags          { D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE },
        .NodeMask       { 0 },
      };
      TAC_CALL(mGpuDescriptorHeapCBV_SRV_UAV.Init( desc, device, errors ));
    }
  }

  // -----------------------------------------------------------------------------------------------

  void              DX12Device::Init( Errors& errors )
  {
    sRenderer.Init( errors );
  }

  PipelineHandle    DX12Device::CreatePipeline( PipelineParams params,
                                                Errors& errors )
  {
    const PipelineHandle h{ AllocPipelineHandle() };
    sRenderer.mPipelineMgr.CreatePipeline( h, params, errors );
    return h;
  }

  void              DX12Device::DestroyPipeline( PipelineHandle h )
  {
    sRenderer.mPipelineMgr.DestroyPipeline( h );
  }

  ProgramHandle     DX12Device::CreateProgram( ProgramParams params, Errors& errors )
  {
    const ProgramHandle h{ AllocProgramHandle() };
    sRenderer.mProgramMgr.CreateProgram( h, params, errors );
    return h;
  }

  void              DX12Device::DestroyProgram( ProgramHandle h )
  {
    sRenderer.mProgramMgr.DestroyProgram( h );
  }

  SwapChainHandle   DX12Device::CreateSwapChain( SwapChainParams params, Errors& errors )
  {
    const SwapChainHandle h{ AllocSwapChainHandle() };
    sRenderer.mSwapChainMgr.CreateSwapChain( h, params, errors );
    return h;
  }

  void              DX12Device::ResizeSwapChain( SwapChainHandle h, v2i size )
  {
    sRenderer.mSwapChainMgr.ResizeSwapChain( h, size );
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

  BufferHandle      DX12Device::CreateBuffer( CreateBufferParams params,
                                              Errors& errors )
  {
    const BufferHandle h{ AllocBufferHandle() };
    sRenderer.mBufMgr.CreateBuffer( h, params, errors );
    return h;
  }

  void              DX12Device::UpdateBuffer( BufferHandle h, UpdateBufferParams params )
  {
    sRenderer.mBufMgr.UpdateBuffer( h, params );
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
    const TextureHandle h{ AllocTextureHandle() };
    sRenderer.mTexMgr.CreateTexture( h, params, errors );
    return h;
  }

  void              DX12Device::UpdateTexture( TextureHandle h, UpdateTextureParams params )
  {
    sRenderer.mTexMgr.UpdateTexture( h, params );
  }

  void              DX12Device::DestroyTexture( TextureHandle h )
  {
    sRenderer.mTexMgr.DestroyTexture( h );
  }

} // namespace Tac::Render

