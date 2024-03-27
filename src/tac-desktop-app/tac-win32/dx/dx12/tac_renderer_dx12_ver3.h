#pragma once

#include "tac-std-lib/tac_ints.h" // u64
#include "tac-std-lib/containers/tac_array.h"
#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-win32/dx/dxgi/tac_dxgi.h"
#include "tac-rhi/render3/tac_render_backend.h"

#include "tac_dx12_device.h"
#include "tac_dx12_info_queue.h"
#include "tac_dx12_debug_layer.h"

#include "tac_dx12_samplers.h"
#include "tac_dx12_command_queue.h"
#include "tac_dx12_descriptor_heap.h"
#include "tac_dx12_command_allocator_pool.h"
#include "tac_dx12_context_manager.h"
#include "tac_dx12_gpu_upload_allocator.h"


#include <d3d12.h> // D3D12...

namespace Tac { struct Errors; }

namespace Tac::Render
{
  /*

  struct DX12CommandList : public ICommandList
  {
    void Draw() override;

    DX12Context mContext;
  };
  */


  /*
  struct DX12Window
  {
  };
  */

  struct DX12Resource
  {
    PCom< ID3D12Resource > mResource;
    D3D12_RESOURCE_DESC    mDesc{};
    D3D12_RESOURCE_STATES  mState{};
  };

  struct DX12DynBuf
  {
    DX12Resource mResource;
    void*        mMappedCPUAddr = nullptr;
  };

  const int TAC_SWAP_CHAIN_BUF_COUNT = 3;

  using SwapChainRTVs = Array< DX12Resource, TAC_SWAP_CHAIN_BUF_COUNT >;

  struct DX12FrameBuf
  {
    const void*             mNWH{};
    v2i                     mSize{};
    DX12CommandQueue        mCommandQueue;
    PCom< IDXGISwapChain4 > mSwapChain;
    DXGI_SWAP_CHAIN_DESC1   mSwapChainDesc;
    SwapChainRTVs           mRTVs;
    TexFmt                  mFmt = TexFmt::kUnknown;
  };

  // you know, do we even inherit form renderer?
  // this is our chance to rebuild the renderer
  struct DX12Backend : public IRenderBackend3
  {
    void Init( Errors& ) override;

    DX12Device                 mDevice;
    DX12DebugLayer             mDebugLayer;
    DX12InfoQueue              mInfoQueue;

    void   CreateFB( FBHandle, FrameBufferParams, Errors& ) override;
    void   ResizeFB( FBHandle, v2i )  override;
    TexFmt GetFBFmt( FBHandle ) override;
    void   DestroyFB( FBHandle)  override;

    void CreateDynBuf( DynBufHandle, int, StackFrame, Errors& ) override;
    void UpdateDynBuf( RenderApi::UpdateDynBufParams ) override;
    void DestroyDynBuf( DynBufHandle) override;

    void CreateShaderProgram( ProgramHandle, ShaderProgramParams, Errors& ) override;
    void DestroyShaderProgram( ProgramHandle ) override;

    void CreateRenderPipeline( PipelineHandle, PipelineParams, Errors& ) override;
    void DestroyRenderPipeline( PipelineHandle ) override;

    DX12FrameBuf mFrameBufs[ 100 ];
    DX12DynBuf mDynBufs[ 100 ];

    DX12DescriptorHeap         mRTVDescriptorHeap;


/*
    void CreateDynamicBuffer2( const DynBufCreateParams&, Errors& ) override;
    void UpdateDynamicBuffer2( const DynBufUpdateParams& ) override;
    void SetRenderObjectName( const SetRenderObjectNameParams& ) override;

    Cmds GetCommandList( ContextHandle, Errors& ) override;

    // ---------------------------------------------------------------------------------------------

    // Frame timings

    // Index of the render target that
    // 1. our commands will be drawing onto
    // 2. our swap chain will present to the monitor
    int                        m_backbufferIndex{};

    // total number of frames sent to the gpu
    u64                        mSentGPUFrameCount = 0;

    // index of the next gpu frame to be in-flight ( see also MAX_GPU_FRAME_COUNT )
    u64                        m_gpuFlightFrameIndex = 0;


    DX12CommandQueue           mCommandQueue;
    DX12CommandAllocatorPool   mCommandAllocatorPool;
    DX12ContextManager         mContextManager;
    DX12DescriptorHeap         mSRVDescriptorHeap;
    DX12DescriptorHeap         mSamplerDescriptorHeap;
    DX12Samplers               mSamplers;
    DX12UploadPageMgr          mUploadPageManager;
    Vector< FenceSignal >      mFenceValues;
    Vector< DX12Context >      mContexts;
    Vector< DX12DynBuf >       mDynBufs;
    */
  };
} // namespace Tac::Render
