#pragma once

#include "tac-std-lib/tac_ints.h" // u64
#include "tac-std-lib/containers/tac_array.h"
#include "tac-rhi/render3/tac_render_backend.h"
//#include "tac-rhi/render3/tac_render_api.h"
#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-dx/dxgi/tac_dxgi.h"
#include "tac-dx/dx12/program/tac_dx12_program_mgr.h"
#include "tac-dx/dx12/pipeline/tac_dx12_pipeline_mgr.h"
#include "tac-dx/dx12/buffer/tac_dx12_buffer_mgr.h"
#include "tac-dx/dx12/texture/tac_dx12_texture_mgr.h"
#include "tac-dx/dx12/buffer/tac_dx12_frame_buf_mgr.h"
#include "tac-dx/dx12/sampler/tac_dx12_sampler_mgr.h"
#include "tac-dx/dx12/device/tac_dx12_device.h"
#include "tac-dx/dx12/device/tac_dx12_info_queue.h"
#include "tac-dx/dx12/device/tac_dx12_debug_layer.h"
//#include "tac-dx/dx12/program/tac_dx12_program_bindings.h"
#include "tac-dx/dx12/tac_dx12_command_queue.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap.h"
#include "tac-dx/dx12/tac_dx12_command_allocator_pool.h"
#include "tac-dx/dx12/context/tac_dx12_context_manager.h"
#include "tac-dx/dx12/tac_dx12_gpu_upload_allocator.h"


#include <d3d12.h> // D3D12...

namespace Tac { struct Errors; }

namespace Tac::Render
{

  struct DX12Renderer
  {
    void Init( Errors& );
    void InitDescriptorHeaps( Errors& );

    DX12CommandQueue           mCommandQueue;
    DX12CommandAllocatorPool   mCommandAllocatorPool;
    DX12ContextManager         mContextManager;
    DX12UploadPageMgr          mUploadPageManager;
    DX12DeviceInitializer      mDeviceInitializer;
    DX12DebugLayer             mDebugLayer;
    DX12InfoQueue              mInfoQueue;

    // Managers
    DX12SwapChainMgr           mSwapChainMgr;
    DX12BufferMgr              mBufMgr;
    DX12TextureMgr             mTexMgr;
    DX12ProgramMgr             mProgramMgr;
    DX12PipelineMgr            mPipelineMgr;
    DX12SamplerMgr             mSamplerMgr;

    // CPU Heaps (used for creating resources)
    DX12DescriptorHeap         mCpuDescriptorHeapRTV;
    DX12DescriptorHeap         mCpuDescriptorHeapDSV;
    DX12DescriptorHeap         mCpuDescriptorHeapCBV_SRV_UAV;
    DX12DescriptorHeap         mCpuDescriptorHeapSampler;

    // GPU Heaps (used for rendering)
    DX12DescriptorHeap         mGpuDescriptorHeapCBV_SRV_UAV;
    DX12DescriptorHeap         mGpuDescriptorHeapSampler;

    ID3D12Device* mDevice{};
  };

  struct DX12Device : public IDevice
  {
    void Init( Errors& ) override;

    Info            GetInfo() const override;

    PipelineHandle  CreatePipeline( PipelineParams, Errors& ) override;
    IShaderVar*     GetShaderVariable( PipelineHandle, StringView ) override;
    void            DestroyPipeline( PipelineHandle ) override;

    ProgramHandle   CreateProgram( ProgramParams, Errors& ) override;
    void            DestroyProgram( ProgramHandle ) override;

    SamplerHandle   CreateSampler( CreateSamplerParams ) override;
    void            DestroySampler( SamplerHandle ) override;

    SwapChainHandle CreateSwapChain( SwapChainParams, Errors& ) override;
    void            ResizeSwapChain( SwapChainHandle, v2i, Errors& ) override;
    SwapChainParams GetSwapChainParams( SwapChainHandle ) override;
    void            DestroySwapChain( SwapChainHandle ) override;
    TextureHandle   GetSwapChainCurrentColor( SwapChainHandle ) override;
    TextureHandle   GetSwapChainDepth( SwapChainHandle ) override;
    void            Present( SwapChainHandle, Errors& ) override;

    BufferHandle    CreateBuffer( CreateBufferParams, Errors& ) override;
    void            DestroyBuffer( BufferHandle ) override;

    TextureHandle   CreateTexture( CreateTextureParams, Errors& ) override;
    void            DestroyTexture( TextureHandle ) override;

    IContext::Scope CreateRenderContext( Errors& ) override;

  private:


/*
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


    */
    /*
    DX12Samplers               mSamplers;
    Vector< FenceSignal >      mFenceValues;
    Vector< DX12Context >      mContexts;
    */
  };
} // namespace Tac::Render
