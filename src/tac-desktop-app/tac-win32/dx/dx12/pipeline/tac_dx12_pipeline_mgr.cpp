#include "tac_dx12_pipeline_mgr.h" // self-inc

#include "tac-win32/dx/dx12/program/tac_dx12_program_mgr.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h"
#include "tac-win32/dx/dxgi/tac_dxgi.h"
#include "tac-win32/dx/dx12/pipeline/tac_dx12_root_sig_builder.h"
#include "tac-win32/dx/dx12/pipeline/tac_dx12_input_layout.h"

//#include "tac-win32/dx/hlsl/tac_hlsl_preprocess.h"
//#include "tac-win32/dx/dxc/tac_dxc.h"
//#include "tac-win32/dx/dx12/tac_dx12_helper.h" // TAC_DX12_CALL
//#include "tac-win32/dx/dx12/tac_dx12_root_sig_bindings.h" // D3D12RootSigBindings
#include "tac-std-lib/error/tac_error_handling.h"
//#include "tac-win32/dx/dx12/tac_dx12_root_sig_builder.h"

#if !TAC_DELETE_ME()
#include "tac-std-lib/os/tac_os.h"
#endif

namespace Tac::Render
{




  void DX12PipelineMgr::Init( ID3D12Device* device, DX12ProgramMgr* programMgr )
  {
    mDevice = device;
    mProgramMgr = programMgr;
  }

  void DX12PipelineMgr::DestroyPipeline( PipelineHandle h )
  {
    if( h.IsValid() )
      mPipelines[ h.GetIndex() ] = {};
  }

  void DX12PipelineMgr::CreatePipeline( PipelineHandle h, PipelineParams params, Errors& errors )
  {
    ID3D12Device* device{ mDevice };
    DX12Program* program{ mProgramMgr->FindProgram( params.mProgram ) };

    const D3D12_RASTERIZER_DESC RasterizerState
    {
      .FillMode              { D3D12_FILL_MODE_SOLID },
      .CullMode              { D3D12_CULL_MODE_BACK },
      .FrontCounterClockwise { true },
      .DepthBias             { D3D12_DEFAULT_DEPTH_BIAS },
      .DepthBiasClamp        { D3D12_DEFAULT_DEPTH_BIAS_CLAMP },
      .SlopeScaledDepthBias  { D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS },
      .DepthClipEnable       { true },
    };

    const D3D12_RENDER_TARGET_BLEND_DESC RenderTargetBlendDesc
    {
      .RenderTargetWriteMask { D3D12_COLOR_WRITE_ENABLE_ALL },
    };

    const UINT NumRenderTargets{ ( UINT )params.mRTVColorFmts.size() };

    D3D12_BLEND_DESC BlendState{};
    for( UINT i{}; i < NumRenderTargets; ++i )
      BlendState.RenderTarget[ i ] = RenderTargetBlendDesc;

    const bool hasInputLayout{ !params.mVtxDecls.empty() };

    DX12RootSigBuilder rootSigBuilder( device );
    rootSigBuilder.SetInputLayoutEnabled( hasInputLayout );
    rootSigBuilder.AddBindings( program->mProgramBindings.data(),
                                program->mProgramBindings.size() );

    TAC_CALL( PCom< ID3D12RootSignature > rootSig{ rootSigBuilder.Build( errors ) } );

    const DXGI_SAMPLE_DESC SampleDesc{ .Count { 1 } };
    const DXGI_FORMAT DSVFormat{ TexFmtToDxgiFormat( params.mDSVDepthFmt ) };
    
    const DX12InputLayout inputLayout( params.mVtxDecls, program );

    TAC_NOT_CONST D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc
    {
      .pRootSignature        { ( ID3D12RootSignature* )rootSig },
      .VS                    { program->mVSBytecode },
      .PS                    { program->mPSBytecode },
      .BlendState            { BlendState },
      .SampleMask            { UINT_MAX },
      .RasterizerState       { RasterizerState },
      .DepthStencilState     { D3D12_DEPTH_STENCIL_DESC{} },
      .InputLayout           { inputLayout },
      .PrimitiveTopologyType { D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
      .NumRenderTargets      { NumRenderTargets },
      .DSVFormat             { DSVFormat },
      .SampleDesc            { SampleDesc },
    };

    const int n { params.mRTVColorFmts.size() };
    for( int i{}; i < n; ++i )
      psoDesc.RTVFormats[ i ] = TexFmtToDxgiFormat( params.mRTVColorFmts[ i ] );

    PCom< ID3D12PipelineState > pso;
    TAC_CALL( device->CreateGraphicsPipelineState( &psoDesc, pso.iid(), pso.ppv() ) );

    ID3D12PipelineState* pPS{ pso.Get() };
    const DX12Name name
    {
      .mName          { params.mName },
      .mStackFrame    { params.mStackFrame },
      .mResourceType  { "PSO" },
      .mResourceIndex { h.GetIndex() },
    };
    DX12SetName( pPS, name );

    Vector< DX12Pipeline::Variable > shaderVariables;
    for( const D3D12ProgramBinding& binding : program->mProgramBindings )
      shaderVariables.push_back( DX12Pipeline::Variable( &binding ) );

    mPipelines[ h.GetIndex() ] = DX12Pipeline
    {
      .mPSO             { pso },
      .mRootSignature   { rootSig },
      .mShaderVariables { shaderVariables },
    };
  }

  DX12Pipeline* DX12PipelineMgr::FindPipeline( PipelineHandle h )
  {
    return h.IsValid() ? &mPipelines[ h.GetIndex() ] : nullptr;
  }
} // namespace Tac::Render

