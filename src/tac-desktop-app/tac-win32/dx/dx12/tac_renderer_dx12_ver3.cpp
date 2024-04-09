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

#if 0
  // -----------------------------------------------------------------------------------------------

  // DX12CommandList

  void DX12CommandList::Draw()
  {
    ID3D12GraphicsCommandList* cmdList = mContext.GetCommandList();
    const UINT vtxCountPerInstance = 0;
    const UINT instanceCount = 0;
    const UINT startVertexLocation = 0;
    const UINT startIndexLocation = 0;
    cmdList->DrawInstanced( vtxCountPerInstance,
                            instanceCount,
                            startVertexLocation,
                            startIndexLocation );
  }

  // -----------------------------------------------------------------------------------------------

  // DX12DynBuf

  void DX12DynBuf::SetName( StringView name )
  {
    DX12SetName( mResource, name );
  }

  // -----------------------------------------------------------------------------------------------

  // DX12Backend
#endif

  // -----------------------------------------------------------------------------------------------

  //DX12Context::~DX12Context()
  //{
  //}

  //void DX12Context::SetViewport( v2i size )
  //{
  //}

  //void DX12Context::SetScissor( v2i size )
  //{
  //}

  //void DX12Context::SetRenderTarget( FBHandle h )
  //{
  //}

  // -----------------------------------------------------------------------------------------------

  //const int TAC_MAX_FB_COUNT = 100;

  void    DX12Backend::InitDescriptorHeaps( Errors& errors )
  {
    ID3D12Device* device = mDevice.GetID3D12Device();

    // CPU RTV
    {
      D3D12_DESCRIPTOR_HEAP_DESC desc
      {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = 25,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        .NodeMask = 0,
      };
      TAC_CALL(mCpuDescriptorHeapRTV.Init( desc, device, errors ));
    }

    // CPU DSV
    {
      D3D12_DESCRIPTOR_HEAP_DESC desc
      {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        .NumDescriptors = 25,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        .NodeMask = 0,
      };
      TAC_CALL(mCpuDescriptorHeapDSV.Init( desc, device, errors ));
    }

    // CPU CBV SRV UAV
    {
      D3D12_DESCRIPTOR_HEAP_DESC desc
      {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = 100,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        .NodeMask = 0,
      };
      TAC_CALL(mCpuDescriptorHeapCBV_SRV_UAV.Init( desc, device, errors ));
    }

    // CPU Sampler
    {
      D3D12_DESCRIPTOR_HEAP_DESC desc
      {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
        .NumDescriptors = 10,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        .NodeMask = 0,
      };
      TAC_CALL(mCpuDescriptorHeapSampler.Init( desc, device, errors ));
    }


    // GPU Sampler
    {
      D3D12_DESCRIPTOR_HEAP_DESC desc
      {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
        .NumDescriptors = 10,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        .NodeMask = 0,
      };
      TAC_CALL(mGpuDescriptorHeapSampler.Init( desc, device, errors ));
    }

    // GPU CBV SRV UAV
    {
      D3D12_DESCRIPTOR_HEAP_DESC desc
      {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = 100,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        .NodeMask = 0,
      };
      TAC_CALL(mGpuDescriptorHeapCBV_SRV_UAV.Init( desc, device, errors ));
    }
  }

  void    DX12Backend::Init( Errors& errors )
  {
    TAC_CALL( DXGIInit( errors ) );

    TAC_CALL( mDebugLayer.Init( errors ) );

    TAC_CALL( mDevice.Init( mDebugLayer, errors ) );

    ID3D12Device* device = mDevice.GetID3D12Device();

    TAC_CALL( mInfoQueue.Init( mDebugLayer, device, errors ) );

    TAC_CALL( InitDescriptorHeaps( errors ) );

    TAC_CALL( mProgramMgr.Init( device, errors ) );

    TAC_CALL( mPipelineMgr.Init( device, &mProgramMgr ) );

    mFrameBufMgr.Init( device, &mCommandQueue, &mCpuDescriptorHeapRTV );

    mBufMgr.Init( device );

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
    TAC_CALL( mCommandQueue.Create( device, errors ) );
    mCommandAllocatorPool.Init( device, &mCommandQueue );
    mContextManager.Init( &mCommandAllocatorPool,
                          &mCommandQueue,
                          &mUploadPageManager,
                          &mFrameBufMgr,
                          device );
    mUploadPageManager.Init( device, &mCommandQueue );
    /*

    TAC_CALL( mSRVDescriptorHeap.InitSRV( 100, device, errors ) );
    TAC_CALL( mSamplerDescriptorHeap.InitSampler( 100, device, errors ) );



    mSamplers.Init( device, &mSamplerDescriptorHeap );
    */
  }

#if 0

  SmartPtr< ICommandList > DX12Backend::GetCommandList( ContextHandle handle, Errors& errors )
  {
    const int i = handle.GetHandleIndex();
    if( !( i < mContexts.size() ) )
    {
      mContexts.resize( i + 1 );
    }

    DX12Context context = mContextManager.GetContextNoScope( errors );
    mContexts[ i ] = context;

    DX12CommandList* dx12CmdList = TAC_NEW DX12CommandList;
    dx12CmdList->mContext = context;

    return SmartPtr< ICommandList >{ dx12CmdList };
  }

#endif

  void    DX12Backend::CreateRenderPipeline( PipelineHandle h,
                                             PipelineParams params,
                                             Errors& errors )
  {
    mPipelineMgr.CreatePipeline( h, params, errors );

  }

  void    DX12Backend::DestroyRenderPipeline( PipelineHandle h )
  {
    mPipelineMgr.DestroyPipeline( h );
  }

  void    DX12Backend::CreateProgram( ProgramHandle h,
                                      ProgramParams params,
                                      Errors& errors )
  {
    mProgramMgr.CreateProgram( h, params, errors );
  }

  void    DX12Backend::DestroyProgram( ProgramHandle h )
  {
    mProgramMgr.DestroyProgram( h );
  }

  void    DX12Backend::CreateFB( FBHandle h,
                                 FrameBufferParams params,
                                 Errors& errors )
  {
    mFrameBufMgr.CreateFB( h, params, errors );
  }

  void    DX12Backend::ResizeFB( FBHandle h, v2i size )
  {
    mFrameBufMgr.ResizeFB( h, size );
  }

  TexFmt  DX12Backend::GetFBFmt( FBHandle h )
  {
    return mFrameBufMgr.GetFBFmt( h );
  }

  void    DX12Backend::DestroyFB( FBHandle h )
  {
    return mFrameBufMgr.DestroyFB( h);
  }

  void    DX12Backend::CreateDynBuf( DynBufHandle h,
                                     int byteCount,
                                     StackFrame sf,
                                     Errors& errors )
  {
    mBufMgr.CreateDynBuf( h, byteCount, sf, errors );
  }

  void    DX12Backend::UpdateDynBuf( RenderApi::UpdateDynBufParams params )
  {
    mBufMgr.UpdateDynBuf( params );
  }

  void    DX12Backend::DestroyDynBuf( DynBufHandle h )
  {
    mBufMgr.DestroyDynBuf( h );
  }

  IContextBackend* DX12Backend::CreateRenderContextBackend(Errors& errors)
  {
    return mContextManager.GetContext(errors);
  }

} // namespace Tac::Render
