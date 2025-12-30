#include "tac_dx12_pipeline_mgr.h" // self-inc

#include "tac-dx/dx12/program/tac_dx12_program_mgr.h"
#include "tac-dx/dx12/tac_dx12_helper.h"
#include "tac-dx/dx12/pipeline/tac_dx12_root_sig_builder.h"
#include "tac-dx/dx12/pipeline/tac_dx12_input_layout.h"
#include "tac-dx/dx12/tac_dx12_command_queue.h"
#include "tac-dx/dx12/tac_renderer_dx12_ver3.h"
#include "tac-dx/dxgi/tac_dxgi.h"
#include "tac-rhi/render3/tac_render_backend.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac::Render
{
  static auto GetDX12DepthFunc( DepthFunc depthFunc ) -> D3D12_COMPARISON_FUNC
  {
    switch( depthFunc )
    {
    case DepthFunc::Less:        return D3D12_COMPARISON_FUNC_LESS;
    case DepthFunc::LessOrEqual: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    default: TAC_ASSERT_INVALID_CASE( depthFunc ); return D3D12_COMPARISON_FUNC_LESS;
    }
  }

  static auto GetDX12FillMode( FillMode fillMode ) -> D3D12_FILL_MODE
  {
    switch( fillMode )
    {
    case FillMode::Solid:     return D3D12_FILL_MODE_SOLID;
    case FillMode::Wireframe: return D3D12_FILL_MODE_WIREFRAME;
    default: TAC_ASSERT_INVALID_CASE( fillMode ); return D3D12_FILL_MODE_SOLID;
    }
  }

  static auto GetDX12CullMode( CullMode cullMode ) -> D3D12_CULL_MODE
  {
    switch( cullMode )
    {
    case CullMode::None:  return D3D12_CULL_MODE_NONE;
    case CullMode::Back:  return D3D12_CULL_MODE_BACK;
    case CullMode::Front: return D3D12_CULL_MODE_FRONT;
    default: TAC_ASSERT_INVALID_CASE( cullMode ); return D3D12_CULL_MODE_NONE;
    }
  }

  static auto GetDX12Blend( BlendConstants blendConstants ) -> D3D12_BLEND
  {
    switch( blendConstants )
    {
    case BlendConstants::One:          return D3D12_BLEND_ONE;
    case BlendConstants::Zero:         return D3D12_BLEND_ZERO;
    case BlendConstants::SrcRGB:       return D3D12_BLEND_SRC_COLOR;
    case BlendConstants::SrcA:         return D3D12_BLEND_SRC_ALPHA;
    case BlendConstants::OneMinusSrcA: return D3D12_BLEND_INV_SRC_ALPHA;
    default: TAC_ASSERT_INVALID_CASE( blendConstants ); return D3D12_BLEND_ONE;
    }
  }

  static auto GetDX12BlendOp( BlendMode blendMode ) -> D3D12_BLEND_OP
  {
    switch( blendMode )
    {
    case BlendMode::Add: return D3D12_BLEND_OP_ADD;
    default: TAC_ASSERT_INVALID_CASE( blendMode ); return D3D12_BLEND_OP_ADD;
    }
  }

  static auto GetDX12DepthWriteMask( bool depthWrite ) -> D3D12_DEPTH_WRITE_MASK
  {
    return depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
  }

  static auto GetDX12PrimTopo( PrimitiveTopology top ) -> D3D12_PRIMITIVE_TOPOLOGY_TYPE
  {
    switch( top )
    {
    case PrimitiveTopology::LineList:     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    case PrimitiveTopology::TriangleList: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    case PrimitiveTopology::PointList:    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    default: TAC_ASSERT_INVALID_CASE( top ); return {};
    }
  }


  // -----------------------------------------------------------------------------------------------

  void DX12PipelineMgr::DestroyPipeline( PipelineHandle h )
  {
    if( h.IsValid() )
    {
      FreeHandle( h );
      mPipelines[ h.GetIndex() ] = {};
    }
  }

  void DX12PipelineMgr::CreatePipelineAtIndex( PipelineHandle h,
                                                         PipelineParams params,
                                                         Errors& errors )
  {
    TAC_ASSERT( params.mProgram.IsValid() );

    ID3D12Device* device{ DX12Renderer::sRenderer. mDevice };
    DX12ProgramMgr* programMgr{ &DX12Renderer::sRenderer.mProgramMgr };
    DX12Program* program{ programMgr->FindProgram( params.mProgram ) };

    const D3D12_RASTERIZER_DESC RasterizerState
    {
      .FillMode              { GetDX12FillMode( params.mRasterizerState.mFillMode ) },
      .CullMode              { GetDX12CullMode( params.mRasterizerState.mCullMode ) },
      .FrontCounterClockwise { params.mRasterizerState.mFrontCounterClockwise },
      .DepthBias             { D3D12_DEFAULT_DEPTH_BIAS },
      .DepthBiasClamp        { D3D12_DEFAULT_DEPTH_BIAS_CLAMP },
      .SlopeScaledDepthBias  { D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS },
      .DepthClipEnable       { true },
      .MultisampleEnable     { params.mRasterizerState.mMultisample },
    };

    const D3D12_RENDER_TARGET_BLEND_DESC RenderTargetBlendDesc
    {
      .BlendEnable           { TRUE },
      .SrcBlend              { GetDX12Blend( params.mBlendState.mSrcRGB ) },
      .DestBlend             { GetDX12Blend( params.mBlendState.mDstRGB ) },
      .BlendOp               { GetDX12BlendOp( params.mBlendState.mBlendRGB ) },
      .SrcBlendAlpha         { GetDX12Blend( params.mBlendState.mSrcA ) },
      .DestBlendAlpha        { GetDX12Blend( params.mBlendState.mDstA ) },
      .BlendOpAlpha          { GetDX12BlendOp( params.mBlendState.mBlendA ) },
      .RenderTargetWriteMask { D3D12_COLOR_WRITE_ENABLE_ALL },
    };

    const UINT NumRenderTargets{ ( UINT )params.mRTVColorFmts.size() };

    D3D12_BLEND_DESC BlendState{};
    for( UINT i{}; i < NumRenderTargets; ++i )
      BlendState.RenderTarget[ i ] = RenderTargetBlendDesc;

    const bool hasInputLayout{ !params.mVtxDecls.empty() };

    DX12RootSigBuilder rootSigBuilder( device );
    rootSigBuilder.SetInputLayoutEnabled( hasInputLayout );
    rootSigBuilder.AddBindings( program->mProgramBindDescs.data(),
                                program->mProgramBindDescs.size() );

    TAC_CALL( PCom< ID3D12RootSignature > rootSig{ rootSigBuilder.Build( errors ) } );

    const DXGI_SAMPLE_DESC SampleDesc{ .Count { 1 } };
    const DXGI_FORMAT DSVFormat{ DXGIFormatFromTexFmt( params.mDSVDepthFmt ) };
    
    const DX12InputLayout inputLayout( params.mVtxDecls, program );

    const D3D12_DEPTH_STENCIL_DESC depthStencilDesc
    {
      .DepthEnable    { ( BOOL )params.mDepthState.mDepthTest },
      .DepthWriteMask { GetDX12DepthWriteMask( params.mDepthState.mDepthWrite ) },
      .DepthFunc      { GetDX12DepthFunc( params.mDepthState.mDepthFunc ) },
    };

    const D3D12_PRIMITIVE_TOPOLOGY_TYPE topology{ GetDX12PrimTopo( params.mPrimitiveTopology ) };

    const bool isCompute{ program->mCSBlob };
    PCom< ID3D12PipelineState > pso;
    if( isCompute )
    {
      const D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc
      {
        .pRootSignature        { ( ID3D12RootSignature* )rootSig },
        .CS                    { program->mCSBytecode },
      };
      TAC_CALL( device->CreateComputePipelineState( &psoDesc, pso.iid(), pso.ppv() ) );
    }
    else
    {
      dynmc D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc
      {
        .pRootSignature        { ( ID3D12RootSignature* )rootSig },
        .VS                    { program->mVSBytecode },
        .PS                    { program->mPSBytecode },
        .BlendState            { BlendState },
        .SampleMask            { UINT_MAX },
        .RasterizerState       { RasterizerState },
        .DepthStencilState     { depthStencilDesc },
        .InputLayout           { inputLayout },
        .PrimitiveTopologyType { topology },
        .NumRenderTargets      { NumRenderTargets },
        .DSVFormat             { DSVFormat },
        .SampleDesc            { SampleDesc },
      };

      const int n{ params.mRTVColorFmts.size() };
      for( int i{}; i < n; ++i )
        psoDesc.RTVFormats[ i ] = DXGIFormatFromTexFmt( params.mRTVColorFmts[ i ] );

      TAC_CALL( device->CreateGraphicsPipelineState( &psoDesc, pso.iid(), pso.ppv() ) );
    }

    ID3D12PipelineState* pPS{ pso.Get() };

    DX12NameHelper
    {
      .mName          { params.mName },
      .mStackFrame    { params.mStackFrame },
      .mHandle        { h },
    }.NameObject( pPS );

    const PipelineBindCache pipelineBindCache( program->mProgramBindDescs );
    const DX12Pipeline::Variables shaderVariables( h, pipelineBindCache.size() );

    const int iPipeline{ h.GetIndex() };
    mPipelines[ iPipeline ] = DX12Pipeline
    {
      .mPSO               { pso },
      .mRootSignature     { rootSig },
      .mShaderVariables   { shaderVariables },
      .mPipelineParams    { params },
      .mPipelineBindCache { ( PipelineBindCache&& )pipelineBindCache },
      .mIsCompute         { isCompute },
    };
  }

  auto DX12PipelineMgr::CreatePipeline( PipelineParams params, Errors& errors ) -> PipelineHandle
  {
    const PipelineHandle h{ AllocPipelineHandle() };
    CreatePipelineAtIndex( h, params, errors );
    return h;
  }

  auto DX12PipelineMgr::FindPipeline( PipelineHandle h ) -> DX12Pipeline*
  {
    return h.IsValid() ? &mPipelines[ h.GetIndex() ] : nullptr;
  }

  void DX12PipelineMgr::HotReload( Span< ProgramHandle > changedPrograms, Errors& errors )
  {
    if( changedPrograms.empty() )
      return;

    DX12CommandQueue* commandQueue{ &DX12Renderer::sRenderer.mCommandQueue };

    // try prevent error OBJECT_DELETED_WHILE_STILL_IN_USE when deleting a pipeline that referenced
    // by in-flight operations on a command queue
    TAC_CALL( commandQueue->WaitForIdle( errors ) );

    const int n{ mPipelines.size() };
    for( int i{}; i < n; ++i )
    {
      DX12Pipeline& pipeline{ mPipelines[ i ] };
      if( !pipeline.IsValid() )
        continue;

      bool programChanged{};
      for( ProgramHandle changedProgram : changedPrograms )
        if( pipeline.mPipelineParams.mProgram.GetIndex() == changedProgram.GetIndex() )
          programChanged = true;

      if( !programChanged )
        continue;

      const IShaderVar* pVarsPrev{ pipeline.mShaderVariables.data() };
      const int nVarsPrev{ pipeline.mShaderVariables.size() };

      PipelineHandle h{ i };
      PipelineBindCache prevBindCache{ pipeline.mPipelineBindCache };
      TAC_CALL( CreatePipelineAtIndex( h, pipeline.mPipelineParams, errors ) );
      pipeline.mPipelineBindCache = prevBindCache;

      const IShaderVar* pVarsCurr{ pipeline.mShaderVariables.data() };
      const int nVarsCurr{ pipeline.mShaderVariables.size() };

      // check that IShaderVar* out in the wild are still valid
      TAC_ASSERT( pVarsCurr == pVarsPrev );
      TAC_ASSERT( nVarsCurr == nVarsPrev );
    }
  }

} // namespace Tac::Render

