#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_optional.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-dx/dx12/tac_dx12_gpu_upload_allocator.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap_allocation.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-rhi/render3/tac_render_backend.h"

#include <d3d12.h> // ID3D12...

namespace Tac { struct Errors; }

namespace Tac::Render
{
  struct DX12CommandAllocatorPool;
  struct DX12ContextManager;
  struct DX12CommandQueue;
  struct DX12SwapChainMgr;
  struct DX12TextureMgr;
  struct DX12BufferMgr;
  struct DX12PipelineMgr;
  struct DX12SamplerMgr;
  struct DX12DescriptorHeap;
}

namespace Tac::Render
{

  // A context has a commandlist, even if the context is recycled, the commandlist stays with it
  // forever.
  //
  // However, the commandallocator is changed every time the context is recycled
  struct DX12Context : public IContext
  {
    DX12Context() = default;

    // note(n473): i dont like how with dx12context::Begin and dx12context::Finish,
    // there is no protection (afaict) to prevent someone from forgetting to call Finish.
    ID3D12GraphicsCommandList* GetCommandList();
    ID3D12CommandAllocator*    GetCommandAllocator();
    DX12Descriptor             AllocGPUDescriptors( D3D12_DESCRIPTOR_HEAP_TYPE );

    struct Params
    {
      PCom< ID3D12GraphicsCommandList > mCommandList                  {};
      DX12UploadPageMgr*                mUploadPageManager            {};
      DX12CommandAllocatorPool*         mCommandAllocatorPool         {};
      DX12ContextManager*               mContextManager               {};
      DX12CommandQueue*                 mCommandQueue                 {};
      DX12SwapChainMgr*                 mSwapChainMgr                 {};
      DX12TextureMgr*                   mTextureMgr                   {};
      DX12BufferMgr*                    mBufferMgr                    {};
      DX12PipelineMgr*                  mPipelineMgr                  {};
      DX12SamplerMgr*                   mSamplerMgr                   {};
      DX12DescriptorHeap*               mGpuDescriptorHeapCBV_SRV_UAV {};
      DX12DescriptorHeap*               mGpuDescriptorHeapSampler     {};
      ID3D12Device*                     mDevice                       {};
    };

    void Init( Params );

    //void SetName( StringView );
    void Reset( Errors& );
    void Execute( Errors& ) override;
    void SetSynchronous() override;
    void SetViewport( v2i ) override;
    void SetScissor( v2i ) override;
    void SetRenderTargets( Targets ) override;
    void SetPrimitiveTopology( PrimitiveTopology ) override;
    void SetPipeline( PipelineHandle ) override;
    void ClearColor( TextureHandle, v4 ) override;
    void ClearDepth( TextureHandle, float ) override;
    void SetVertexBuffer( BufferHandle ) override;
    void SetIndexBuffer( BufferHandle ) override;
    void Draw( DrawArgs ) override;
    void DebugEventBegin( StringView ) override;
    void DebugEventEnd() override;
    void DebugMarker( StringView ) override;
    void MoveFrom( DX12Context&& ) noexcept;
    void UpdateBuffer( BufferHandle, Span< const UpdateBufferParams >, Errors& ) override;
    void UpdateTexture( TextureHandle, UpdateTextureParams, Errors& ) override;
    void CommitShaderVariables() override;
    void Retire() override;

    // begin state

    using RenderTargetColors = FixedVector< D3D12_CPU_DESCRIPTOR_HANDLE, 10 >;
    using RenderTargetDepth = Optional< D3D12_CPU_DESCRIPTOR_HANDLE >;
    using GPUDescriptors = Vector< DX12Descriptor >[ D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES ];
    using DX12DescriptorHeaps = Array< DX12DescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES >;

    struct State
    {
      RenderTargetColors mRenderTargetColors   {};
      RenderTargetDepth  mRenderTargetDepth    {}; 
      BufferHandle       mVertexBuffer         {};
      BufferHandle       mIndexBuffer          {};
      PipelineHandle     mPipeline             {};
      bool               mSynchronous          {};
      bool               mExecuted             {};
      int                mEventCount           {};
      bool               mRetired              {};
      GPUDescriptors     mGPUDescs             {};
    };

    State mState{};


    // end state

    PCom< ID3D12GraphicsCommandList > mCommandList                  {};
    PCom< ID3D12CommandAllocator >    mCommandAllocator             {};

    // ok so like this needs to be owned so different command lists dont mix up their upload memory
    DX12UploadAllocator               mGPUUploadAllocator           {};

    // singletons
    DX12CommandAllocatorPool*         mCommandAllocatorPool         {};
    DX12ContextManager*               mContextManager               {};
    DX12CommandQueue*                 mCommandQueue                 {};
    DX12SwapChainMgr*                 mSawpChainMgr                 {};
    DX12TextureMgr*                   mTextureMgr                   {};
    DX12BufferMgr*                    mBufferMgr                    {};
    DX12PipelineMgr*                  mPipelineMgr                  {};
    DX12SamplerMgr*                   mSamplerMgr                   {};
    DX12DescriptorHeaps               mGpuDescriptorHeaps           {};
    ID3D12Device*                     mDevice                       {};
  };

}
