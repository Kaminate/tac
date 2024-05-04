#include "tac_dx12_pipeline_mgr.h" // self-inc

#include "tac-win32/dx/dx12/program/tac_dx12_program_mgr.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h"
#include "tac-win32/dx/dxgi/tac_dxgi.h"
#include "tac-win32/dx/dx12/pipeline/tac_dx12_root_sig_builder.h"

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
  static D3D12_DESCRIPTOR_RANGE_TYPE
    D3D12ProgramBindingType_To_D3D12_DESCRIPTOR_RANGE_TYPE( D3D12ProgramBinding::Type type )
  {
    switch( type )
    {
    case D3D12ProgramBinding::Type::kTextureUAV:
    case D3D12ProgramBinding::Type::kBufferUAV:
      return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;

    case D3D12ProgramBinding::Type::kTextureSRV:
    case D3D12ProgramBinding::Type::kBufferSRV:
      return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

    case D3D12ProgramBinding::Type::kSampler:
      return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;

    case D3D12ProgramBinding::Type::kConstantBuffer:
      return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;

    default: TAC_ASSERT_INVALID_CASE( type ); return ( D3D12_DESCRIPTOR_RANGE_TYPE )0;
    }
  }

  static D3D12_ROOT_PARAMETER_TYPE
    D3D12ProgramBindingType_To_D3D12_ROOT_PARAMETER_TYPE( D3D12ProgramBinding::Type type )
  {
    switch( type )
    {
    case D3D12ProgramBinding::Type::kTextureUAV:
    case D3D12ProgramBinding::Type::kBufferUAV:
      return D3D12_ROOT_PARAMETER_TYPE_UAV;

    case D3D12ProgramBinding::Type::kTextureSRV:
    case D3D12ProgramBinding::Type::kBufferSRV:
      return D3D12_ROOT_PARAMETER_TYPE_SRV;

    case D3D12ProgramBinding::Type::kConstantBuffer:
      return D3D12_ROOT_PARAMETER_TYPE_CBV;

    default: TAC_ASSERT_INVALID_CASE( type ); return ( D3D12_ROOT_PARAMETER_TYPE )0;
    }
  }


  static PCom< ID3D12RootSignature > BuildRootSignature( ID3D12Device* device,
                                                         const D3D12ProgramBindings& bindings,
                                                         Errors& errors )
  {
    DX12RootSigBuilder rootSigBuilder( device );

    for( const D3D12ProgramBinding& binding : bindings.mBindings )
    {

      const DX12RootSigBuilder::Location loc
      {
        .mRegister { binding.mBindRegister },
        .mSpace    { binding.mRegisterSpace },
      };

      const bool isArray { binding.mBindCount != 1 };

      if( isArray || binding.mType == D3D12ProgramBinding::Type::kSampler )
      {
        // Create a root descriptor table
        const D3D12_DESCRIPTOR_RANGE_TYPE descriptorRangeType =
          D3D12ProgramBindingType_To_D3D12_DESCRIPTOR_RANGE_TYPE( binding.mType );

        if( binding.mBindCount == 0 )
          rootSigBuilder.AddUnboundedArray( descriptorRangeType, loc );
        else
          rootSigBuilder.AddBoundedArray( descriptorRangeType, binding.mBindCount, loc );
      }
      else
      {
        // Create a root descriptor
        const D3D12_ROOT_PARAMETER_TYPE type = 
          D3D12ProgramBindingType_To_D3D12_ROOT_PARAMETER_TYPE( binding.mType );
        rootSigBuilder.AddRootDescriptor( type, loc );
      }

    }

    return rootSigBuilder.Build( errors );
  }

#if 0
  static void ShaderInputToRootParam( D3D12_SHADER_INPUT_BIND_DESC& info,
                                      DX12RootSigBuilder* rootSigBuilder )
  {

    // UINT_MAX represents an unbounded array
    const UINT NumDescriptors = info.BindCount == 0 ? UINT_MAX : info.BindCount;

    if( info.Type == D3D_SIT_SAMPLER || NumDescriptors != 1 )
    {
      D3D12_DESCRIPTOR_RANGE_TYPE RangeType = ShaderInputToDescriptorRangeType( info.Type );
      D3D12_DESCRIPTOR_RANGE1 range {
        .RangeType = RangeType,
        .NumDescriptors = NumDescriptors,
        .BaseShaderRegister = info.BindPoint,
        .RegisterSpace = info.Space,
        .Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
        .OffsetInDescriptorsFromTableStart = 0,
      };
      rootSigBuilder->AddRootDescriptorTable( D3D12_SHADER_VISIBILITY_ALL, range );
    }
    else
    {

      D3D12_ROOT_PARAMETER_TYPE ParameterType = ShaderInputToRootParamType( info.Type );
      TAC_ASSERT( ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV ||
                  ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV ||
                  ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV );

      D3D12_ROOT_DESCRIPTOR1 Descriptor
      {
        .ShaderRegister = info.BindPoint,
        .RegisterSpace = info.Space,
        .Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
      };
      rootSigBuilder->AddRootDescriptor( ParameterType, D3D12_SHADER_VISIBILITY_ALL, Descriptor );
    }

  }
#endif


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
    ID3D12Device* device { mDevice };
    DX12Program* program { mProgramMgr->FindProgram( params.mProgram ) };

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


    D3D12_RENDER_TARGET_BLEND_DESC RenderTargetBlendDesc{};
    RenderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    const UINT NumRenderTargets{ ( UINT )params.mRTVColorFmts.size() };

    D3D12_BLEND_DESC BlendState{};
    for( UINT i{}; i < NumRenderTargets; ++i )
      BlendState.RenderTarget[ i ] = RenderTargetBlendDesc;

    TAC_CALL( PCom< ID3D12RootSignature > rootSig{
      BuildRootSignature( device, program->mProgramBindings, errors ) } );

    const DXGI_SAMPLE_DESC SampleDesc{ .Count { 1 } };
    const DXGI_FORMAT DSVFormat{ TexFmtToDxgiFormat( params.mDSVDepthFmt ) };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc
    {
      .pRootSignature        { ( ID3D12RootSignature* )rootSig },
      .VS                    { program->mVSBytecode },
      .PS                    { program->mPSBytecode },
      .BlendState            { BlendState },
      .SampleMask            { UINT_MAX },
      .RasterizerState       { RasterizerState },
      .DepthStencilState     { D3D12_DEPTH_STENCIL_DESC{} },
      .InputLayout           { D3D12_INPUT_LAYOUT_DESC{} },
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

    DX12SetName( pso, "Pipeline State" + Tac::ToString( h.GetIndex() ) );

    mPipelines[ h.GetIndex() ] = DX12Pipeline
    {
      .mPSO           { pso },
      .mRootSignature { rootSig },
    };
  }

  DX12Pipeline* DX12PipelineMgr::FindPipeline( PipelineHandle h )
  {
    return h.IsValid() ? &mPipelines[ h.GetIndex() ] : nullptr;
  }
} // namespace Tac::Render

