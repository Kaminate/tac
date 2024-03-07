#pragma once

#include "src/common/tac_ints.h" // u64
#include "src/shell/windows/tac_win32_com_ptr.h" // PCom
#include "src/common/graphics/render/tac_render_backend.h"
#include "src/common/graphics/render/tac_render_command_list.h"

#include "tac_dx12_command_queue.h"
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

  struct DX12DescriptorHeap
  {
    PCom< ID3D12DescriptorHeap >       mHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE        mCpuHeapStart{};
    D3D12_GPU_DESCRIPTOR_HANDLE        mGpuHeapStart{};
  };

  struct DX12CommandList : public ICommandList
  {
    DX12Context mContext;
    void Draw() override;
  };


  // you know, do we even inherit form renderer?
  // this is our chance to rebuild the renderer
  struct DX12Backend : public IBackend
  {
    void Init( Errors& ) override;

    void CreateDynamicBuffer2( const DynBufCreateParams& ) override;
    Cmds GetCommandList( ContextHandle, Errors& ) override;

    PCom< ID3D12Device >               m_device0;

    // ---------------------------------------------------------------------------------------------

    // Frame timings

    // Index of the render target that
    // 1. our commands will be drawing onto
    // 2. our swap chain will present to the monitor
    int                                m_backbufferIndex{};

    // total number of frames sent to the gpu
    u64                                mSentGPUFrameCount = 0;

    // index of the next gpu frame to be in-flight ( see also MAX_GPU_FRAME_COUNT )
    u64                                m_gpuFlightFrameIndex = 0;

    DX12CommandQueue                   mCommandQueue;
    DX12CommandAllocatorPool           mCommandAllocatorPool;
    DX12ContextManager                 mContextManager;
    GPUUploadPageManager               mUploadPageManager;

    UINT                               m_descriptorSizes[ D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES ]{};

    Vector<FenceSignal>                mFenceValues;
    Vector<DX12Context>                mContexts;
  };
}
