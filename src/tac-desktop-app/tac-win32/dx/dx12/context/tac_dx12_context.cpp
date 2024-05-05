#include "tac_dx12_context.h" // self-inc

#include "tac-win32/dx/dx12/tac_dx12_command_allocator_pool.h"
#include "tac-win32/dx/dx12/tac_dx12_gpu_upload_allocator.h"
#include "tac-win32/dx/dx12/texture/tac_dx12_texture_mgr.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h"
#include "tac-win32/dx/dx12/tac_renderer_dx12_ver3.h"
#include "tac-win32/dx/dx12/buffer/tac_dx12_frame_buf_mgr.h"

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/containers/tac_fixed_vector.h"

#include <WinPixEventRuntime/pix3.h>

#if !TAC_DELETE_ME()
#include "tac-std-lib/os/tac_os.h"
#endif


namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  void DX12Context::UpdateTexture( TextureHandle h, UpdateTextureParams params, Errors& errors )
  {
    mTextureMgr->UpdateTexture( h, params, this, errors );
  }

  void DX12Context::UpdateBuffer( BufferHandle h, UpdateBufferParams params, Errors& errors )
  {
    mBufferMgr->UpdateBuffer( h, params, this, errors );
  }


  void DX12Context::Execute( Errors& errors )
  {
    TAC_ASSERT( !mState.mExecuted );

    ID3D12GraphicsCommandList* commandList { GetCommandList() };
    if( !commandList )
      return; // This context has been (&&) moved

    while( mState.mEventCount )
      DebugEventEnd();

    // Indicates that recording to the command list has finished.
    TAC_DX12_CALL( commandList->Close() );

    const FenceSignal fenceSignal = TAC_CALL(
      mCommandQueue->ExecuteCommandList( commandList, errors ) );

    if( mState.mSynchronous )
    {
      mCommandQueue->WaitForFence( fenceSignal, errors );
      TAC_ASSERT( !errors );
    }

    mCommandAllocatorPool->Retire( mCommandAllocator, fenceSignal );
    mCommandAllocator = {};
    mGPUUploadAllocator.FreeAll( fenceSignal );

    mState.mExecuted = true;
  }

  ID3D12GraphicsCommandList* DX12Context::GetCommandList() { return mCommandList.Get(); }
  ID3D12CommandAllocator* DX12Context::GetCommandAllocator() { return mCommandAllocator.Get(); }

  void DX12Context::Reset( Errors& errors )
  {
    //mGPUUploadAllocator; // <-- should be clear

    TAC_ASSERT( !mCommandAllocator );

    mCommandAllocator =
      TAC_CALL( mCommandAllocatorPool->GetAllocator( errors ) );

    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    //
    // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandallocator-reset
    // ID3D12CommandAllocator::Reset
    //   Indicates to re-use the memory that is associated with the command allocator.
    //   From this call to Reset, the runtime and driver determine that the GPU is no longer
    //   executing any command lists that have recorded commands with the command allocator.
    ID3D12CommandAllocator* dxCommandAllocator { GetCommandAllocator() };
    TAC_DX12_CALL( mCommandAllocator->Reset() );


    // However( when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    //
    // ID3D12GraphicsCommandList::Reset
    //
    //   Resets a command list back to its initial state as if a new command list was just created.
    //   After Reset succeeds, the command list is left in the "recording" state.
    //
    //   you can re-use command list tracking structures without any allocations
    //   you can call Reset while the command list is still being executed
    //   you can submit a cmd list, reset it, and reuse the allocated memory for another cmd list
    ID3D12GraphicsCommandList* dxCommandList { GetCommandList() };
    TAC_DX12_CALL( dxCommandList->Reset( dxCommandAllocator, nullptr ) );

    mState = {};
  }

  void DX12Context::SetName( StringView name )
  {
    DX12SetName( mCommandAllocator, name );
    DX12SetName( mCommandList, name );
  }

  void DX12Context::SetSynchronous()
  {
    mState.  mSynchronous = true;
  }

  void DX12Context::SetViewport( v2i size )
  {
    ID3D12GraphicsCommandList* cmd { GetCommandList() };
    const D3D12_VIEWPORT vp
    {
      .TopLeftX { 0 },
      .TopLeftY { 0 },
      .Width    { ( FLOAT )size.x },
      .Height   { ( FLOAT )size.y },
      .MinDepth { 0 },
      .MaxDepth { 1 },
    };
    cmd->RSSetViewports( 1, &vp );
  }

  void DX12Context::SetScissor( v2i size )
  {
    ID3D12GraphicsCommandList* cmd { GetCommandList() };
    const D3D12_RECT rect
    {
      .right  { ( LONG )size.x },
      .bottom { ( LONG )size.y },
    };
    cmd->RSSetScissorRects( 1, &rect );
  }

  void DX12Context::DebugEventBegin( StringView str )
  {
    ID3D12GraphicsCommandList* commandList { GetCommandList() };
    PIXBeginEvent( commandList, PIX_COLOR_DEFAULT, str );
    mState.mEventCount++;
  }

  void DX12Context::DebugEventEnd()
  {
    TAC_ASSERT( mState.mEventCount > 0 );
    ID3D12GraphicsCommandList* commandList { GetCommandList() };
    PIXEndEvent( commandList );
    mState.mEventCount--;
  }

  void DX12Context::MoveFrom( DX12Context&& ) noexcept
  {
    TAC_ASSERT_UNIMPLEMENTED;
  }

  void DX12Context::DebugMarker( StringView str )
  {
    ID3D12GraphicsCommandList* commandList { GetCommandList() };
    PIXSetMarker( commandList, PIX_COLOR_DEFAULT, str );
  }

  void DX12Context::SetRenderTargets( Targets targets )
  {
    FixedVector< D3D12_CPU_DESCRIPTOR_HANDLE, 10 > rtDescs;
    FixedVector< D3D12_RESOURCE_BARRIER, 10 > barriers;

    for( TextureHandle colorTarget : targets.mColors )
    {
      if( DX12Texture * colorTexture{ mTextureMgr->FindTexture( colorTarget ) } )
      {
        const D3D12_RESOURCE_STATES StateBefore { colorTexture->mState };
        const D3D12_RESOURCE_STATES StateAfter { D3D12_RESOURCE_STATE_RENDER_TARGET };
        if( StateBefore != StateAfter )
        {
          colorTexture->mState = StateAfter;

          const D3D12_RESOURCE_TRANSITION_BARRIER Transition
          {
            .pResource   { colorTexture->mResource.Get() },
            .Subresource { D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES },
            .StateBefore { StateBefore },
            .StateAfter  { StateAfter },
          };

          const D3D12_RESOURCE_BARRIER barrier
          {
            .Type       { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION },
            .Flags      { D3D12_RESOURCE_BARRIER_FLAG_NONE },
            .Transition { Transition },
          };
          barriers.push_back( barrier );
        }

        rtDescs.push_back( colorTexture->mRTV->GetCPUHandle() );
      }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DSV{};
    D3D12_CPU_DESCRIPTOR_HANDLE* pDSV{};
    if( DX12Texture* depthTexture{ mTextureMgr->FindTexture( targets.mDepth ) } )
    {
      DSV = depthTexture->mRTV->GetCPUHandle();
      pDSV = &DSV;
      mState.mRenderTargetDepth = DSV;
    }

    mState.mRenderTargetColors = rtDescs;

    ID3D12GraphicsCommandList* commandList { GetCommandList() };
    if( !barriers.empty() )
      commandList->ResourceBarrier( ( UINT )barriers.size(), barriers.data() );
    commandList->OMSetRenderTargets( ( UINT )rtDescs.size(), rtDescs.data(), false, pDSV );
  }

  void DX12Context::SetPrimitiveTopology( PrimitiveTopology primitiveTopology )
  {
    ID3D12GraphicsCommandList* commandList { GetCommandList() };
    D3D12_PRIMITIVE_TOPOLOGY dx12Topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

        D3D_PRIMITIVE_TOPOLOGY_UNDEFINED	= 0,
        D3D_PRIMITIVE_TOPOLOGY_POINTLIST	= 1,
        D3D_PRIMITIVE_TOPOLOGY_LINELIST	= 2,
        D3D_PRIMITIVE_TOPOLOGY_LINESTRIP	= 3,
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST	= 4,
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP	= 5,
        D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ	= 10,
        D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ	= 11,
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ	= 12,
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ	= 13,

    commandList->IASetPrimitiveTopology( dx12Topology );
  }

  void DX12Context::SetPipeline( PipelineHandle h )
  {
    DX12Pipeline* pipeline { mPipelineMgr->FindPipeline( h ) };
    ID3D12PipelineState* pipelineState { pipeline->mPSO.Get() };
    ID3D12RootSignature* rootSignature { pipeline->mRootSignature.Get() };
    ID3D12GraphicsCommandList* commandList { GetCommandList() };
    commandList->SetPipelineState( pipelineState );
    commandList->SetGraphicsRootSignature( rootSignature );
  }

  void DX12Context::ClearColor( TextureHandle h, v4 values )
  {
    const DX12Texture* texture{ mTextureMgr->FindTexture( h ) };
    TAC_ASSERT( texture );

    const D3D12_CPU_DESCRIPTOR_HANDLE RTV{ texture->mRTV->GetCPUHandle() };

    ID3D12GraphicsCommandList* commandList { GetCommandList() };
    commandList->ClearRenderTargetView( RTV, values.data(), 0, nullptr );
  }

  void DX12Context::ClearDepth( TextureHandle h, float value )
  {
    DX12Texture* texture{ mTextureMgr->FindTexture( h ) };
    TAC_ASSERT( texture );
    TAC_ASSERT( texture->mDSV.HasValue() );
    
    const D3D12_CPU_DESCRIPTOR_HANDLE DSV { texture->mDSV.GetValueUnchecked().GetCPUHandle() };
    const D3D12_CLEAR_FLAGS ClearFlags { D3D12_CLEAR_FLAG_DEPTH };
    const FLOAT Depth { 1.0f };

    ID3D12GraphicsCommandList* commandList { GetCommandList() };
    const D3D12_RESOURCE_STATES StateBefore { texture->mState };
    const D3D12_RESOURCE_STATES StateAfter { D3D12_RESOURCE_STATE_DEPTH_WRITE };

    if( StateBefore != StateAfter )
    {
      texture->mState = StateAfter;

      const D3D12_RESOURCE_TRANSITION_BARRIER Transition
      {
        .pResource   { texture->mResource.Get() },
        .Subresource { D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES },
        .StateBefore { StateBefore },
        .StateAfter  { StateAfter },
      };

      const D3D12_RESOURCE_BARRIER barrier
      {
        .Type       { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION },
        .Flags      { D3D12_RESOURCE_BARRIER_FLAG_NONE },
        .Transition { Transition },
      };

      commandList->ResourceBarrier( 1, &barrier );
    }

    commandList->ClearDepthStencilView( DSV, ClearFlags,Depth, 0, 0, nullptr );
  }

  void DX12Context::Draw( DrawArgs args )
  {
    ID3D12GraphicsCommandList* commandList { GetCommandList() };
    if( mState.mIndexBuffer.IsValid() )
    {
      const D3D12_DRAW_INDEXED_ARGUMENTS dx12DrawArgs
      {
        // Is it weird to pass the "vertexcount" parameter to an "indexcount" value?
        .IndexCountPerInstance { (UINT)args.mVertexCount },

        .InstanceCount         { 1 },
        .StartIndexLocation    {},
        .BaseVertexLocation    {},
        .StartInstanceLocation {},
      };

      commandList->DrawIndexedInstanced( dx12DrawArgs.IndexCountPerInstance,
                                         dx12DrawArgs.InstanceCount,
                                         dx12DrawArgs.StartIndexLocation,
                                         dx12DrawArgs.BaseVertexLocation,
                                         dx12DrawArgs.StartInstanceLocation );
    }
    else
    {
      const D3D12_DRAW_ARGUMENTS dx12DrawArgs
      {
        .VertexCountPerInstance { ( UINT )args.mVertexCount },
        .InstanceCount          { 1 },
        .StartVertexLocation    {},
        .StartInstanceLocation  {},
      };
      commandList->DrawInstanced( dx12DrawArgs.VertexCountPerInstance,
                                  dx12DrawArgs.InstanceCount,
                                  dx12DrawArgs.StartVertexLocation,
                                  dx12DrawArgs.StartInstanceLocation );
    }
  }

  void DX12Context::Retire()
  {
    mContextManager->RetireContext( this );
  }

} // namespace Tac::Render
