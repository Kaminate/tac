#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_optional.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-dx/dx12/tac_dx12_gpu_upload_allocator.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap_allocation.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_allocator.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_cache.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-rhi/render3/tac_render_backend.h"

#include <d3d12.h> // ID3D12...

namespace Tac { struct Errors; }

namespace Tac::Render
{
  struct DX12ContextManager;
}

namespace Tac::Render
{

  // A context has a commandlist, even if the context is recycled, the commandlist stays with it
  // forever.
  //
  // However, the commandallocator is changed every time the context is recycled
  struct DX12Context : public IContext
  {
    struct Params
    {
      PCom< ID3D12GraphicsCommandList > mCommandList    {};
      DX12ContextManager*               mContextManager {};
    };

    ID3D12GraphicsCommandList* GetCommandList();
    ID3D12CommandAllocator*    GetCommandAllocator();

    DX12Context() = default;
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
    void Dispatch( v3i ) override;
    void DebugEventBegin( StringView ) override;
    void DebugEventEnd() override;
    void DebugMarker( StringView ) override;
    void MoveFrom( DX12Context&& ) noexcept;
    void UpdateBuffer( BufferHandle, Span< const UpdateBufferParams >, Errors& ) override;
    void UpdateTexture( TextureHandle, UpdateTextureParams, Errors& ) override;
    void CommitShaderVariables() override;
    void Retire() override;

    struct State
    {
      using RenderTargetColors = FixedVector< D3D12_CPU_DESCRIPTOR_HANDLE, 10 >;
      using RenderTargetDepth = Optional< D3D12_CPU_DESCRIPTOR_HANDLE >;

      RenderTargetColors    mRenderTargetColors   {};
      RenderTargetDepth     mRenderTargetDepth    {}; 
      BufferHandle          mVertexBuffer         {};
      BufferHandle          mIndexBuffer          {};
      PipelineHandle        mPipeline             {};
      bool                  mIsCompute            {};
      bool                  mSynchronous          {};
      bool                  mExecuted             {};
      int                   mEventCount           {};
      bool                  mRetired              {};
      DX12DescriptorCache   mDescriptorCache      {};
    };

    State mState{};

    PCom< ID3D12GraphicsCommandList > mCommandList                  {};
    PCom< ID3D12CommandAllocator >    mCommandAllocator             {};

    // ok so like this needs to be owned so different command lists dont mix up their upload memory
    DX12UploadAllocator               mGPUUploadAllocator           {};
  };

} // namespace Tac::Render
