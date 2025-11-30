#pragma once

#include "tac-std-lib/tac_ints.h" // u64
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/containers/tac_fifo_queue.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/shell/tac_shell_time.h"
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
      ResourceHandle mResourceHandle      {};
      u64            mRenderFrameIndex    { RenderApi::GetCurrentRenderFrameIndex() };

      // why is this needed?
      GameFrame     mSimulationFrameIndex { GameTimer::GetElapsedFrames() };
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
    void Init( Errors& ) override;
    void Uninit() override;
    Info GetInfo() const override;
    void BeginRenderFrame( Errors& ) override;
    void EndRenderFrame( Errors& ) override;
    auto CreatePipeline( PipelineParams, Errors& ) -> PipelineHandle override;
    auto GetShaderVariable( PipelineHandle, StringView ) -> IShaderVar*override;
    void DestroyPipeline( PipelineHandle ) override;
    auto CreateProgram( ProgramParams, Errors& ) -> ProgramHandle override;
    auto GetProgramBindings_TEST( ProgramHandle ) -> String override;
    void DestroyProgram( ProgramHandle ) override;
    auto CreateSampler( CreateSamplerParams ) -> SamplerHandle override;
    void DestroySampler( SamplerHandle ) override;
    auto CreateSwapChain( SwapChainParams, Errors& ) -> SwapChainHandle override;
    void ResizeSwapChain( SwapChainHandle, v2i, Errors& ) override;
    auto GetSwapChainColorFmt( SwapChainHandle ) -> TexFmt override;
    auto GetSwapChainDepthFmt( SwapChainHandle ) -> TexFmt override;
    void DestroySwapChain( SwapChainHandle ) override;
    auto GetSwapChainCurrentColor( SwapChainHandle ) -> TextureHandle override;
    auto GetSwapChainDepth( SwapChainHandle ) -> TextureHandle override;
    void Present( SwapChainHandle, Errors& ) override;
    auto CreateBuffer( CreateBufferParams, Errors& ) -> BufferHandle override;
    void DestroyBuffer( BufferHandle ) override;
    auto CreateTexture( CreateTextureParams, Errors& ) -> TextureHandle override;
    void DestroyTexture( TextureHandle ) override;
    auto CreateRenderContext( Errors& ) -> IContext::Scope override;
    auto CreateBindlessArray( IBindlessArray::Params ) -> IBindlessArray*override;
  };
} // namespace Tac::Render
