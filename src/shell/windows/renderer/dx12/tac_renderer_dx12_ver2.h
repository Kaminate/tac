#pragma once

#include "tac-std-lib/tac_ints.h" // u64
#include "src/shell/windows/tac_win32_com_ptr.h" // PCom
#include "tac-rhi/render/tac_render_backend.h"
#include "tac-rhi/render/tac_render_command_list.h"

#include "tac_dx12_device.h"
#include "tac_dx12_info_queue.h"
#include "tac_dx12_samplers.h"
#include "tac_dx12_debug_layer.h"
#include "tac_dx12_command_queue.h"
#include "tac_dx12_descriptor_heap.h"
#include "tac_dx12_command_allocator_pool.h"
#include "tac_dx12_context_manager.h"
#include "tac_dx12_gpu_upload_allocator.h"

#include <d3d12.h> // D3D12...

namespace Tac
{
  struct Errors;
}

namespace Tac::Render
{
  const int SWAP_CHAIN_BUFFER_COUNT = 3;

  struct DX12CommandList : public ICommandList
  {
    void Draw() override;

    DX12Context mContext;
  };

  struct DX12DynBuf
  {
    void SetName( StringView );

    PCom< ID3D12Resource > mResource;
    void*                  mMappedCPUAddr = nullptr;
  };

  // you know, do we even inherit form renderer?
  // this is our chance to rebuild the renderer
  struct DX12Backend : public IBackend
  {
    void Init( Errors& ) override;

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

    DX12Device                 mDevice;
    DX12DebugLayer             debugLayer;
    DX12InfoQueue              infoQueue;

    DX12CommandQueue           mCommandQueue;
    DX12CommandAllocatorPool   mCommandAllocatorPool;
    DX12ContextManager         mContextManager;
    DX12DescriptorHeap         mRTVDescriptorHeap;
    DX12DescriptorHeap         mSRVDescriptorHeap;
    DX12DescriptorHeap         mSamplerDescriptorHeap;
    DX12Samplers               mSamplers;
    DX12UploadPageMgr          mUploadPageManager;
    Vector< FenceSignal >      mFenceValues;
    Vector< DX12Context >      mContexts;
    Vector< DX12DynBuf >       mDynBufs;
  };
} // namespace Tac::Render
