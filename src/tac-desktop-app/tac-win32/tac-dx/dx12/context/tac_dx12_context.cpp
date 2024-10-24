#include "tac_dx12_context.h" // self-inc

#include "tac-dx/dx12/tac_dx12_command_allocator_pool.h"
#include "tac-dx/dx12/tac_dx12_gpu_upload_allocator.h"
#include "tac-dx/dx12/texture/tac_dx12_texture_mgr.h"
#include "tac-dx/dx12/tac_dx12_helper.h"
#include "tac-dx/dx12/tac_renderer_dx12_ver3.h"
#include "tac-dx/dx12/buffer/tac_dx12_frame_buf_mgr.h"
#include "tac-dx/dx12/tac_dx12_transition_helper.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_allocator.h"
#include "tac-dx/pix/tac_pix_runtime.h"


#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/containers/tac_fixed_vector.h"
#include "tac-std-lib/algorithm/tac_algorithm.h" // Swap


namespace Tac::Render
{
  static D3D12_PRIMITIVE_TOPOLOGY   GetDX12PrimitiveTopology( PrimitiveTopology topology )
  {
    switch( topology )
    {
    case PrimitiveTopology::Unknown:              return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    case PrimitiveTopology::TriangleList:         return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case PrimitiveTopology::PointList:            return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case PrimitiveTopology::LineList:             return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    default: TAC_ASSERT_INVALID_CASE( topology ); return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }
  }


  // -----------------------------------------------------------------------------------------------

  void DX12Context::Init( Params params )
  {
    mCommandList = params.mCommandList;
    TAC_ASSERT( mCommandList );

    mGPUUploadAllocator.Init( &DX12Renderer::sRenderer.mUploadPageManager );

    mCommandAllocatorPool = &DX12Renderer::sRenderer.mCommandAllocatorPool;
    TAC_ASSERT( mCommandAllocatorPool );

    mContextManager = &DX12Renderer::sRenderer.mContextManager;
    TAC_ASSERT( mContextManager );

    mCommandQueue = &DX12Renderer::sRenderer.mCommandQueue;
    TAC_ASSERT( mCommandQueue );

    mSwapChainMgr = &DX12Renderer::sRenderer.mSwapChainMgr;
    TAC_ASSERT( mSwapChainMgr );

    mTextureMgr = &DX12Renderer::sRenderer.mTexMgr;
    TAC_ASSERT( mTextureMgr );

    mBufferMgr = &DX12Renderer::sRenderer.mBufMgr;
    TAC_ASSERT( mBufferMgr );

    mPipelineMgr = &DX12Renderer::sRenderer.mPipelineMgr;
    TAC_ASSERT( mPipelineMgr );

    mGpuDescriptorHeaps = &DX12Renderer::sRenderer.mGpuDescriptorHeaps;
    TAC_ASSERT( mGpuDescriptorHeaps );

    mSamplerMgr = &DX12Renderer::sRenderer.mSamplerMgr;
    TAC_ASSERT( mSamplerMgr );

    mDevice = DX12Renderer::sRenderer.mDevice;
    TAC_ASSERT( mDevice );
  }

  void DX12Context::CommitShaderVariables()
  {
    DX12Pipeline* pipeline{ mPipelineMgr->FindPipeline( mState.mPipeline ) };
    if( !pipeline )
      return;

    const int iResource{ ( int )D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };
    const int iSampler{ ( int )D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER };

    DX12DescriptorHeap& heap_Resource{ ( *mGpuDescriptorHeaps )[ iResource ] };
    DX12DescriptorHeap& heap_Sampler{ ( *mGpuDescriptorHeaps )[ iSampler ] };

    const Array descHeaps
    {
      heap_Resource.GetID3D12DescriptorHeap(),
      heap_Sampler.GetID3D12DescriptorHeap(),
    };

    mState.mDescriptorCaches[ iResource ].SetRegionManager( heap_Resource.GetRegionMgr() ); // ugly
    mState.mDescriptorCaches[ iResource ].SetRegionManager( heap_Sampler.GetRegionMgr() ); // ugly

    ID3D12GraphicsCommandList* commandList{ GetCommandList() };
    commandList->SetDescriptorHeaps( ( UINT )descHeaps.size(), descHeaps.data() );

    const DX12Pipeline::Variable::CommitParams commitParams
    {
        .mCommandList      {commandList},
        .mDescriptorCaches {&mState.mDescriptorCaches},
        .mIsCompute        {mState.mIsCompute},
    };

    for( const DX12Pipeline::Variable& var : pipeline->mShaderVariables )
      var.Commit( commitParams );
  }

  void DX12Context::UpdateTexture( TextureHandle h,
                                   UpdateTextureParams params,
                                   Errors& errors )
  {
    mTextureMgr->UpdateTexture( h, params, this, errors );
  }

  void DX12Context::UpdateBuffer( BufferHandle h,
                                  Span< const UpdateBufferParams > params,
                                  Errors& errors )
  {
    mBufferMgr->UpdateBuffer( h, params, this, errors );
  }

  void DX12Context::Execute( Errors& errors )
  {
    TAC_ASSERT( !mState.mExecuted );

    ID3D12GraphicsCommandList* commandList{ GetCommandList() };
    if( !commandList )
      return; // This context has been (&&) moved

    while( mState.mEventCount )
      DebugEventEnd();

    // Indicates that recording to the command list has finished.
    TAC_DX12_CALL( commandList->Close() );

    TAC_CALL( const FenceSignal fenceSignal{
      mCommandQueue->ExecuteCommandList( commandList, errors ) }  );

    if( mState.mSynchronous )
    {
      TAC_CALL( mCommandQueue->WaitForFence( fenceSignal, errors ) );
    }


    for( int i{}; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i )
      mState.mDescriptorCaches[ i ].SetFence( fenceSignal );

    mCommandAllocatorPool->Retire( mCommandAllocator, fenceSignal );
    mCommandAllocator = {};
    mGPUUploadAllocator.FreeAll( fenceSignal );
    mState.mExecuted = true;
  }

  ID3D12GraphicsCommandList* DX12Context::GetCommandList() { return mCommandList.Get(); }
  ID3D12CommandAllocator*    DX12Context::GetCommandAllocator() { return mCommandAllocator.Get(); }

  void DX12Context::Reset( Errors& errors )
  {
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

    State empty;
    Swap( mState, empty );
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
      .TopLeftX {  },
      .TopLeftY {  },
      .Width    { ( FLOAT )size.x },
      .Height   { ( FLOAT )size.y },
      .MinDepth {  },
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
    ID3D12GraphicsCommandList* commandList{ GetCommandList() };
    PixRuntimeApi::BeginEvent( commandList, str );
    mState.mEventCount++;
  }

  void DX12Context::DebugEventEnd()
  {
    TAC_ASSERT( mState.mEventCount > 0 );
    ID3D12GraphicsCommandList* commandList{ GetCommandList() };
    PixRuntimeApi::EndEvent( commandList );
    mState.mEventCount--;
  }

  void DX12Context::MoveFrom( DX12Context&& ) noexcept
  {
    TAC_ASSERT_UNIMPLEMENTED;
  }

  void DX12Context::DebugMarker( StringView str )
  {
    ID3D12GraphicsCommandList* commandList{ GetCommandList() };
    PixRuntimeApi::SetMarker( commandList, str );
  }

  void DX12Context::SetRenderTargets( Targets targets )
  {
    FixedVector< D3D12_CPU_DESCRIPTOR_HANDLE, 10 > rtDescs;

    DX12TransitionHelper transitionHelper;

    for( TextureHandle colorTarget : targets.mColors )
    {
      DX12Texture* colorTexture{ mTextureMgr->FindTexture( colorTarget ) };
      if( !colorTexture )
        continue;

      const DX12TransitionHelper::Params transitionParams
      {
        .mResource    { &colorTexture->mResource },
        .mStateAfter  { D3D12_RESOURCE_STATE_RENDER_TARGET },
      };
      transitionHelper.Append( transitionParams );

      TAC_ASSERT( colorTexture->mRTV.HasValue() );
      TAC_ASSERT( colorTexture->mRTV->IsValid() );
      rtDescs.push_back( colorTexture->mRTV->GetCPUHandle() );
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DSV{};
    D3D12_CPU_DESCRIPTOR_HANDLE* pDSV{};
    if( DX12Texture* depthTexture{ mTextureMgr->FindTexture( targets.mDepth ) } )
    {
      DSV = depthTexture->mDSV->GetCPUHandle();
      pDSV = &DSV;
      mState.mRenderTargetDepth = DSV;

      // Assume the the shader has depth write enabled
      const DX12TransitionHelper::Params transitionParams
      {
        .mResource    { &depthTexture->mResource },
        .mStateAfter  { D3D12_RESOURCE_STATE_DEPTH_WRITE },
      };
      transitionHelper.Append( transitionParams );
    }

    mState.mRenderTargetColors = rtDescs;

    ID3D12GraphicsCommandList* commandList{ GetCommandList() };
    transitionHelper.ResourceBarrier( commandList );
    commandList->OMSetRenderTargets( ( UINT )rtDescs.size(), rtDescs.data(), false, pDSV );
  }

  void DX12Context::SetPrimitiveTopology( PrimitiveTopology primitiveTopology )
  {
    ID3D12GraphicsCommandList* commandList { GetCommandList() };
    const D3D12_PRIMITIVE_TOPOLOGY dx12Topology { GetDX12PrimitiveTopology( primitiveTopology ) };
    commandList->IASetPrimitiveTopology( dx12Topology );
  }

  void DX12Context::SetPipeline( PipelineHandle h )
  {
    DX12Pipeline* pipeline { mPipelineMgr->FindPipeline( h ) };
    ID3D12PipelineState* pipelineState { pipeline->mPSO.Get() };
    ID3D12RootSignature* rootSignature { pipeline->mRootSignature.Get() };
    ID3D12GraphicsCommandList* commandList { GetCommandList() };
    commandList->SetPipelineState( pipelineState );
    if( pipeline->mIsCompute )
      commandList->SetComputeRootSignature( rootSignature );
    else
      commandList->SetGraphicsRootSignature( rootSignature );
    mState.mPipeline = h;
    mState.mIsCompute = pipeline->mIsCompute;
  }

  void DX12Context::ClearColor( TextureHandle h, v4 values )
  {
    dynmc DX12Texture* texture{ mTextureMgr->FindTexture( h ) };
    TAC_ASSERT( texture );

    dynmc ID3D12GraphicsCommandList* commandList { GetCommandList() };
    const FLOAT* colorRGBA{ values.data() };
    const D3D12_CPU_DESCRIPTOR_HANDLE RTV{ texture->mRTV->GetCPUHandle() };
    const DX12TransitionHelper::Params transitionParams
    {
      .mResource    { &texture->mResource },
      .mStateAfter  { D3D12_RESOURCE_STATE_RENDER_TARGET },
    };

    DX12TransitionHelper transitionHelper;
    transitionHelper.Append( transitionParams );
    transitionHelper.ResourceBarrier( commandList );

    commandList->ClearRenderTargetView( RTV, colorRGBA, 0, nullptr );
  }

  void DX12Context::SetIndexBuffer( BufferHandle h )
  {
    mState.mIndexBuffer = h;

    ID3D12GraphicsCommandList* commandList { GetCommandList() };

    DX12Buffer* buffer{ mBufferMgr->FindBuffer( h ) };
    if( buffer )
    {
      const DXGI_FORMAT Format{ TexFmtToDxgiFormat( buffer->mCreateParams.mGpuBufferFmt ) };
      const D3D12_INDEX_BUFFER_VIEW indexBufferView
      {
        .BufferLocation { buffer->mGPUVirtualAddr },
        .SizeInBytes    { ( UINT )buffer->mCreateParams.mByteCount },
        .Format         { Format },
      };

      commandList->IASetIndexBuffer( &indexBufferView );

    }
    else
    {
      commandList->IASetIndexBuffer( nullptr );
    }

  }

  void DX12Context::SetVertexBuffer( BufferHandle h )
  {
    mState.mVertexBuffer = h;

    DX12Buffer* buffer{ mBufferMgr->FindBuffer( h ) };
    if( !buffer )
      return;

    TAC_ASSERT( Binding{} != ( buffer->mCreateParams.mBinding & Binding::VertexBuffer ) );

    const UINT StartSlot{};
    const UINT NumViews{ 1 };
    const D3D12_VERTEX_BUFFER_VIEW view
    {
      .BufferLocation { buffer->mGPUVirtualAddr },
      .SizeInBytes    { ( UINT )buffer->mCreateParams.mByteCount },
      .StrideInBytes  { ( UINT )buffer->mCreateParams.mStride },
    };

    // When BufferLocation is valid, but SizeInBytes is 0, d3d can throw a
    // COMMAND_LIST_DRAW_VERTEX_BUFFER_NOT_SET warning when drawing
    TAC_ASSERT( view.BufferLocation );
    TAC_ASSERT( view.SizeInBytes );
    TAC_ASSERT( view.StrideInBytes );

    ID3D12GraphicsCommandList* commandList{ GetCommandList() };
    commandList->IASetVertexBuffers( StartSlot, NumViews, &view );
  }

  void DX12Context::ClearDepth( TextureHandle h, float value )
  {
    DX12Texture* texture{ mTextureMgr->FindTexture( h ) };
    TAC_ASSERT( texture );
    TAC_ASSERT( texture->mDSV.HasValue() );
    
    const D3D12_CPU_DESCRIPTOR_HANDLE DSV { texture->mDSV.GetValue().GetCPUHandle() };
    const D3D12_CLEAR_FLAGS ClearFlags { D3D12_CLEAR_FLAG_DEPTH };
    const FLOAT Depth { 1.0f };

    ID3D12GraphicsCommandList* commandList { GetCommandList() };

    const DX12TransitionHelper::Params transitionParams
    {
      .mResource    { &texture->mResource },
      .mStateAfter  { D3D12_RESOURCE_STATE_DEPTH_WRITE },
    };

    DX12TransitionHelper transitionHelper;
    transitionHelper.Append( transitionParams );
    transitionHelper.ResourceBarrier( commandList );

    commandList->ClearDepthStencilView( DSV, ClearFlags,Depth, 0, 0, nullptr );
  }

  void DX12Context::Dispatch( v3i threadGroupCounts )
  {
    const UINT ThreadGroupCountX{ ( UINT )threadGroupCounts.x };
    const UINT ThreadGroupCountY{ ( UINT )threadGroupCounts.y };
    const UINT ThreadGroupCountZ{ ( UINT )threadGroupCounts.z };
    ID3D12GraphicsCommandList* commandList { GetCommandList() };
    commandList->Dispatch( ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ );
  }

  void DX12Context::Draw( DrawArgs args )
  {
    ID3D12GraphicsCommandList* commandList { GetCommandList() };
    if( mState.mIndexBuffer.IsValid() )
    {
      const D3D12_DRAW_INDEXED_ARGUMENTS dx12DrawArgs
      {
        .IndexCountPerInstance { ( UINT )args.mIndexCount },

        .InstanceCount         { 1 },
        .StartIndexLocation    { ( UINT )args.mStartIndex },

        // A value added to each index before reading a vertex from the vertex buffer.
        .BaseVertexLocation    {},
        .StartInstanceLocation {},
      };
      TAC_ASSERT( dx12DrawArgs.IndexCountPerInstance );
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
        .StartVertexLocation    { ( UINT )args.mStartVertex },
        .StartInstanceLocation  {},
      };
      TAC_ASSERT( dx12DrawArgs.VertexCountPerInstance );
      commandList->DrawInstanced( dx12DrawArgs.VertexCountPerInstance,
                                  dx12DrawArgs.InstanceCount,
                                  dx12DrawArgs.StartVertexLocation,
                                  dx12DrawArgs.StartInstanceLocation );
    }
  }

  void DX12Context::Retire()
  {
    TAC_ASSERT( !mState.mRetired );
    TAC_ASSERT( mState.mExecuted ); // this should be a warning instead
    mState.mRetired = true;

    for( int i{}; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i )
      mState.mDescriptorCaches[ i ].Clear();

    mContextManager->RetireContext( this );
  }

} // namespace Tac::Render
