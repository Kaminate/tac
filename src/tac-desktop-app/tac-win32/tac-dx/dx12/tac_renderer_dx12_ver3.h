#pragma once

#include "tac-std-lib/tac_ints.h" // u64
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/containers/tac_fifo_queue.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-rhi/render3/tac_render_backend.h"
#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-dx/dx12/buffer/tac_dx12_buffer_mgr.h"
#include "tac-dx/dx12/buffer/tac_dx12_frame_buf_mgr.h"
#include "tac-dx/dx12/context/tac_dx12_context_manager.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap.h"
#include "tac-dx/dx12/device/tac_dx12_debug_layer.h"
#include "tac-dx/dx12/device/tac_dx12_device.h"
#include "tac-dx/dx12/device/tac_dx12_info_queue.h"
#include "tac-dx/dx12/pipeline/tac_dx12_pipeline_mgr.h"
#include "tac-dx/dx12/program/tac_dx12_program_mgr.h"
#include "tac-dx/dx12/sampler/tac_dx12_sampler_mgr.h"
#include "tac-dx/dx12/tac_dx12_command_allocator_pool.h"
#include "tac-dx/dx12/tac_dx12_command_queue.h"
#include "tac-dx/dx12/tac_dx12_gpu_upload_allocator.h"
#include "tac-dx/dx12/texture/tac_dx12_texture_mgr.h"
#include "tac-dx/dxgi/tac_dxgi.h"


#include <d3d12.h> // D3D12...


namespace Tac::Render
{

  // todo: move to some shared render aux file
  struct DeletionQueue
  {
    void Push( ResourceHandle );
    void Update();

  private:
    struct Entry
    {
      ResourceHandle mResourceHandle;
      u64            mRenderFrameIndex     { RenderApi::GetCurrentRenderFrameIndex() };

      // why is this needed?
      FrameIndex     mSimulationFrameIndex { Timestep::GetElapsedFrames() };
    };

    FifoQueue< Entry > mEntries;
  public:
    bool               mVerbose {};
  };

  struct FrameBufferer
  {
    void BeginRenderFrame( Errors& );
    void EndRenderFrame( Errors& );

    Vector< FenceSignal >      mFrameBufferingFenceValues;
  };

  struct DX12Renderer
  {
    void Init( Errors& );
    void BeginRenderFrame( Errors& );
    void EndRenderFrame( Errors& );

    static DX12Renderer        sRenderer;

    DX12CommandQueue           mCommandQueue;
    DX12CommandAllocatorPool   mCommandAllocatorPool;
    DX12ContextManager         mContextManager;
    DX12UploadPageMgr          mUploadPageManager;
    DX12DeviceInitializer      mDeviceInitializer;
    DX12DebugLayer             mDebugLayer;
    DX12InfoQueue              mInfoQueue;

    // Resource Managers
    DX12SwapChainMgr           mSwapChainMgr;
    DX12BufferMgr              mBufMgr;
    DX12TextureMgr             mTexMgr;
    DX12ProgramMgr             mProgramMgr;
    DX12PipelineMgr            mPipelineMgr;
    DX12SamplerMgr             mSamplerMgr;
    DX12DescriptorHeapMgr      mDescriptorHeapMgr;

    ID3D12Device*              mDevice{};
    DeletionQueue              mDeletionQueue;
    FrameBufferer              mFrameBufferer;
  };

  struct DX12Device : public IDevice
  {
    void            Init( Errors& ) override;
    void            Uninit() override;
    Info            GetInfo() const override;
    void            BeginRenderFrame( Errors& ) override;
    void            EndRenderFrame( Errors& ) override;

    PipelineHandle  CreatePipeline( PipelineParams, Errors& ) override;
    IShaderVar*     GetShaderVariable( PipelineHandle, StringView ) override;
    void            DestroyPipeline( PipelineHandle ) override;

    ProgramHandle   CreateProgram( ProgramParams, Errors& ) override;
    String          GetProgramBindings_TEST( ProgramHandle ) override;
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
    IBindlessArray* CreateBindlessArray( IBindlessArray::Params ) override;
  };
} // namespace Tac::Render
