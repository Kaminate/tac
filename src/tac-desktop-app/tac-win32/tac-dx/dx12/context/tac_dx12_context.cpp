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


  // -----------------------------------------------------------------------------------------------

  void DX12Context::Init( Params params )
  {
    mCommandList = params.mCommandList;
    TAC_ASSERT( mCommandList );

    mGPUUploadAllocator.Init( &DX12Renderer::sRenderer.mUploadPageManager );
  }

  void DX12Context::CommitShaderVariables()
  {
    DX12Renderer& renderer{ DX12Renderer::sRenderer };
    DX12DescriptorHeapMgr& heapMgr{ renderer.mDescriptorHeapMgr };
    DX12PipelineMgr* pipelineMgr{ &renderer.mPipelineMgr };
    TAC_ASSERT( pipelineMgr );


    DX12Pipeline* pipeline{ pipelineMgr->FindPipeline( mState.mPipeline ) };
    if( !pipeline )
      return;

    ID3D12GraphicsCommandList* commandList{ GetCommandList() };

    heapMgr.Bind( commandList );



    for( RootParameterBinding& binding : pipeline->mPipelineBindCache )
    {
      const CommitParams commitParams
      {
          .mCommandList        { commandList },
          .mDescriptorCache    { &mState.mDescriptorCache },
          .mIsCompute          { mState.mIsCompute },
          .mRootParameterIndex { binding.mRootParameterIndex },
      };
      binding.Commit( commitParams );
    }
  }

  void DX12Context::UpdateTexture( TextureHandle h,
                                   UpdateTextureParams params,
                                   Errors& errors )
  {
    DX12TextureMgr* textureMgr{ &DX12Renderer::sRenderer.mTexMgr };
    TAC_ASSERT( textureMgr );
    textureMgr->UpdateTexture( h, params, this, errors );
  }

  void DX12Context::UpdateBuffer( BufferHandle h,
                                  Span< const UpdateBufferParams > params,
                                  Errors& errors )
  {
    DX12BufferMgr* bufferMgr{ &DX12Renderer::sRenderer.mBufMgr };
    TAC_ASSERT( bufferMgr );
    bufferMgr->UpdateBuffer( h, params, this, errors );
  }

  void DX12Context::Execute( Errors& errors )
  {
    TAC_ASSERT( !mState.mExecuted );

    DX12Renderer* renderer{ &DX12Renderer::sRenderer };

    DX12CommandAllocatorPool* commandAllocatorPool{ &renderer->mCommandAllocatorPool };
    TAC_ASSERT( commandAllocatorPool );

    DX12CommandQueue* commandQueue{ &renderer->mCommandQueue };
    TAC_ASSERT( commandQueue );

    ID3D12GraphicsCommandList* commandList{ GetCommandList() };
    if( !commandList )
      return; // This context has been (&&) moved

    while( mState.mEventCount )
      DebugEventEnd();

    // Indicates that recording to the command list has finished.
    TAC_DX12_CALL( commandList->Close() );

    TAC_CALL( const FenceSignal fenceSignal{
      commandQueue->ExecuteCommandList( commandList, errors ) }  );

    if( mState.mSynchronous )
    {
      TAC_CALL( commandQueue->WaitForFence( fenceSignal, errors ) );
    }

    mState.mDescriptorCache.SetFence( fenceSignal );

    DX12PipelineMgr* pipelineMgr{ &renderer->mPipelineMgr };
    TAC_ASSERT( pipelineMgr );
    if( DX12Pipeline* pipeline{ pipelineMgr->FindPipeline( mState.mPipeline ) } )
      for( RootParameterBinding& binding : pipeline->mPipelineBindCache )
        binding.SetFence( fenceSignal );

    commandAllocatorPool->Retire( mCommandAllocator, fenceSignal );
    mCommandAllocator = {};
    mGPUUploadAllocator.FreeAll( fenceSignal );
    mState.mExecuted = true;
  }

  ID3D12GraphicsCommandList* DX12Context::GetCommandList() { return mCommandList.Get(); }
  ID3D12CommandAllocator*    DX12Context::GetCommandAllocator() { return mCommandAllocator.Get(); }

  void DX12Context::Reset( Errors& errors )
  {
    DX12Renderer* renderer{ &DX12Renderer::sRenderer };
    DX12CommandAllocatorPool* commandAllocatorPool{ &renderer->mCommandAllocatorPool };
    TAC_ASSERT( !mCommandAllocator );
    TAC_ASSERT( commandAllocatorPool );

    mCommandAllocator = TAC_CALL( commandAllocatorPool->GetAllocator( errors ) );

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

  void DX12Context::SetSynchronous()
  {
    mState.mSynchronous = true;
  }

  void DX12Context::SetViewport( v2i size )
  {
    ID3D12GraphicsCommandList* cmd { GetCommandList() };
    const D3D12_VIEWPORT vp
    {
      .TopLeftX {},
      .TopLeftY {},
      .Width    { ( FLOAT )size.x },
      .Height   { ( FLOAT )size.y },
      .MinDepth {},
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
    DX12TextureMgr* textureMgr{ &DX12Renderer::sRenderer.mTexMgr };
    FixedVector< D3D12_CPU_DESCRIPTOR_HANDLE, 10 > rtDescs;

    DX12TransitionHelper transitionHelper;

    for( const TextureHandle colorTarget : targets.mColors )
    {
      DX12Texture* colorTexture{ textureMgr->FindTexture( colorTarget ) };
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
    if( dynmc DX12Texture* depthTexture{ textureMgr->FindTexture( targets.mDepth ) } )
    {
      DSV = depthTexture->mDSV->GetCPUHandle();
      pDSV = &DSV;
      mState.mRenderTargetDepth = DSV;

      // Assume the the shader has depth write enabled
      const DX12TransitionHelper::Params transitionParams
      {
        .mResource   { &depthTexture->mResource },
        .mStateAfter { D3D12_RESOURCE_STATE_DEPTH_WRITE },
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
    D3D12_PRIMITIVE_TOPOLOGY topoligies[ ( int )PrimitiveTopology::Count ];
    topoligies[ ( int )PrimitiveTopology::TriangleList ] = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    topoligies[ ( int )PrimitiveTopology::PointList ] = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    topoligies[ ( int )PrimitiveTopology::LineList ] = D3D_PRIMITIVE_TOPOLOGY_LINELIST;

    const D3D12_PRIMITIVE_TOPOLOGY dx12Topology{ topoligies[ ( int )primitiveTopology ] };
    TAC_ASSERT( primitiveTopology != PrimitiveTopology::Unknown ||
                dx12Topology == D3D_PRIMITIVE_TOPOLOGY_UNDEFINED );

    ID3D12GraphicsCommandList* commandList { GetCommandList() };
    commandList->IASetPrimitiveTopology( dx12Topology );
  }

  void DX12Context::SetPipeline( PipelineHandle h )
  {
    using CmdList = ID3D12GraphicsCommandList;

    DX12PipelineMgr* pipelineMgr{ &DX12Renderer::sRenderer.mPipelineMgr };
    TAC_ASSERT( pipelineMgr );
    DX12Pipeline* pipeline { pipelineMgr->FindPipeline( h ) };
    ID3D12PipelineState* pipelineState { pipeline->mPSO.Get() };
    ID3D12RootSignature* rootSignature { pipeline->mRootSignature.Get() };
    ID3D12GraphicsCommandList* commandList { GetCommandList() };
    commandList->SetPipelineState( pipelineState );
    ( commandList ->* ( pipeline->mIsCompute
                        ? &CmdList::SetComputeRootSignature
                        : &CmdList::SetGraphicsRootSignature ) )( rootSignature );
    mState.mPipeline = h;
    mState.mIsCompute = pipeline->mIsCompute;
  }

  void DX12Context::ClearColor( TextureHandle h, v4 values )
  {
    DX12TextureMgr* textureMgr{ &DX12Renderer::sRenderer.mTexMgr };
    dynmc DX12Texture* texture{ textureMgr->FindTexture( h ) };
    TAC_ASSERT( texture );

    dynmc ID3D12GraphicsCommandList* commandList { GetCommandList() };
    const FLOAT* colorRGBA{ values.data() };
    const D3D12_CPU_DESCRIPTOR_HANDLE RTV{ texture->mRTV->GetCPUHandle() };
    const DX12TransitionHelper::Params transitionParams
    {
      .mResource   { &texture->mResource },
      .mStateAfter { D3D12_RESOURCE_STATE_RENDER_TARGET },
    };

    DX12TransitionHelper transitionHelper;
    transitionHelper.Append( transitionParams );
    transitionHelper.ResourceBarrier( commandList );

    commandList->ClearRenderTargetView( RTV, colorRGBA, 0, nullptr );
  }

  void DX12Context::SetIndexBuffer( BufferHandle h )
  {
    DX12BufferMgr* bufferMgr{ &DX12Renderer::sRenderer.mBufMgr };
    TAC_ASSERT( bufferMgr );

    mState.mIndexBuffer = h;

    ID3D12GraphicsCommandList* commandList { GetCommandList() };

    if( DX12Buffer* buffer{ bufferMgr->FindBuffer( h ) } )
    {
      const DXGI_FORMAT Format{ DXGIFormatFromTexFmt( buffer->mCreateParams.mGpuBufferFmt ) };
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
    DX12BufferMgr* bufferMgr{ &DX12Renderer::sRenderer.mBufMgr };
    TAC_ASSERT( bufferMgr );

    mState.mVertexBuffer = h;

    DX12Buffer* buffer{ bufferMgr->FindBuffer( h ) };
    if( !buffer )
      return;

    TAC_ASSERT( buffer->mCreateParams.mBinding & Binding::VertexBuffer );

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
    DX12TextureMgr* textureMgr{ &DX12Renderer::sRenderer.mTexMgr };
    DX12Texture* texture{ textureMgr->FindTexture( h ) };
    TAC_ASSERT( texture );
    if( !texture )
      return;

    TAC_ASSERT( texture->mDSV.HasValue() );
    ID3D12GraphicsCommandList* commandList { GetCommandList() };
    
    const D3D12_CPU_DESCRIPTOR_HANDLE DSV { texture->mDSV.GetValue().GetCPUHandle() };
    const D3D12_CLEAR_FLAGS ClearFlags { D3D12_CLEAR_FLAG_DEPTH };
    const FLOAT Depth { value };
    TAC_ASSERT( value == 1 );

    const DX12TransitionHelper::Params transitionParams
    {
      .mResource    { &texture->mResource },
      .mStateAfter  { D3D12_RESOURCE_STATE_DEPTH_WRITE },
    };

    DX12TransitionHelper transitionHelper;
    transitionHelper.Append( transitionParams );
    transitionHelper.ResourceBarrier( commandList );

    commandList->ClearDepthStencilView( DSV, ClearFlags, Depth, 0, 0, nullptr );
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
    DX12ContextManager* contextManager{ &DX12Renderer::sRenderer.mContextManager };
    TAC_ASSERT( contextManager );
    TAC_ASSERT( !mState.mRetired );
    TAC_ASSERT( mState.mExecuted ); // this should be a warning instead
    mState.mRetired = true;
    contextManager->RetireContext( this );
  }

} // namespace Tac::Render
