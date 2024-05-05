#include "tac_dx12_program_mgr.h" // self-inc

#include "tac-win32/dx/hlsl/tac_hlsl_preprocess.h"
#include "tac-win32/dx/dxc/tac_dxc.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h" // TAC_DX12_CALL
//#include "tac-win32/dx/dx12/program/tac_dx12_program_bindings.h" // D3D12ProgramBindings
#include "tac-std-lib/filesystem/tac_filesystem.h"
//#include "tac-win32/dx/dx12/tac_dx12_root_sig_builder.h"

#if !TAC_DELETE_ME()
#include "tac-std-lib/os/tac_os.h"
#endif

namespace Tac::Render
{
  static const D3D_SHADER_MODEL sShaderModel = D3D_SHADER_MODEL_6_5;

  static D3D12_SHADER_BYTECODE IDxcBlobToBytecode( PCom<IDxcBlob>& blob )
  {
    return blob ?
      D3D12_SHADER_BYTECODE{ blob->GetBufferPointer(), blob->GetBufferSize() } :
      D3D12_SHADER_BYTECODE{};
  }

  static D3D_SHADER_MODEL GetHighestShaderModel( ID3D12Device* device )
  {
    const D3D_SHADER_MODEL lowestDefined { D3D_SHADER_MODEL_5_1 };
    const D3D_SHADER_MODEL highestDefined { D3D_SHADER_MODEL_6_7 }; // D3D_HIGHEST_SHADER_MODEL undefined?;
    for( D3D_SHADER_MODEL shaderModel { highestDefined };
         shaderModel >= lowestDefined;
         shaderModel = D3D_SHADER_MODEL( shaderModel - 1 ) )
    {
      // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_feature_data_shader_model
      //   After the function completes successfully, the HighestShaderModel field contains the
      //   highest shader model that is both supported by the device and no higher than the
      //   shader model passed in.
      TAC_NOT_CONST D3D12_FEATURE_DATA_SHADER_MODEL featureData{ shaderModel };
      if( SUCCEEDED( device->CheckFeatureSupport(
        D3D12_FEATURE_SHADER_MODEL,
        &featureData,
        sizeof( D3D12_FEATURE_DATA_SHADER_MODEL ) ) ) )

        // For some godforsaken fucking reason, this isn't the same as shaderModel
        return featureData.HighestShaderModel;
    }

    return lowestDefined;
  }

#if 0
  static D3D12RootSigBinding::Type ShaderInputToRootSigBindType( D3D_SHADER_INPUT_TYPE Type )
  {
    switch( Type )
    {
    case D3D_SIT_CBUFFER: return D3D12RootSigBinding::Type::kCBuf;
    case D3D_SIT_TEXTURE: return D3D12RootSigBinding::Type::kTexture;
    case D3D_SIT_SAMPLER: return D3D12RootSigBinding::Type::kSampler;
    case D3D_SIT_BYTEADDRESS: return D3D12RootSigBinding::Type::kSRV;
    default: TAC_ASSERT_INVALID_CASE( Type ); return D3D12RootSigBinding::Type::kUnknown;
    }
  }

  static void ShaderInputToRootSigBinding( D3D12_SHADER_INPUT_BIND_DESC& info,
                                           D3D12RootSigBindings* bindings )
  {
  }

  static D3D12_DESCRIPTOR_RANGE_TYPE ShaderInputToDescriptorRangeType( D3D_SHADER_INPUT_TYPE Type )
  {
    switch( Type )
    {
    case D3D_SIT_SAMPLER: return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    case D3D_SIT_CBUFFER: return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    case D3D_SIT_BYTEADDRESS: return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    case D3D_SIT_TEXTURE: return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    default: TAC_ASSERT_INVALID_CASE( Type ); return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    }
  }

  static D3D12_ROOT_PARAMETER_TYPE ShaderInputToRootParamType( D3D_SHADER_INPUT_TYPE Type )
  {
    switch( Type )
    {
    case D3D_SIT_CBUFFER:     return D3D12_ROOT_PARAMETER_TYPE_CBV;
    case D3D_SIT_BYTEADDRESS: return D3D12_ROOT_PARAMETER_TYPE_SRV;
    case D3D_SIT_TEXTURE:     return D3D12_ROOT_PARAMETER_TYPE_SRV;
    case D3D_SIT_SAMPLER:
      TAC_ASSERT_CRITICAL( "Samplers are bound through descriptor heaps "
                           "and cannot be root parameters" );
      return ( D3D12_ROOT_PARAMETER_TYPE )0;
    default: TAC_ASSERT_INVALID_CASE( Type ); return ( D3D12_ROOT_PARAMETER_TYPE )0;
    }
  }

#endif


  // -----------------------------------------------------------------------------------------------

  void DX12ProgramMgr::Init( ID3D12Device* device, Errors& errors )
  {
    mDevice = device;

    const D3D_SHADER_MODEL highestShaderModel { GetHighestShaderModel( device ) };
    TAC_RAISE_ERROR_IF( sShaderModel > highestShaderModel, "Shader model too high" );
  }

  DX12Program* DX12ProgramMgr::FindProgram( ProgramHandle h )
  {
    return h.IsValid() ? &mPrograms[ h.GetIndex() ] : nullptr;
  }

  void DX12ProgramMgr::DestroyProgram( ProgramHandle h )
  {
    if( h.IsValid() )
      mPrograms[ h.GetIndex() ] = {};
  }

  void DX12ProgramMgr::CreateProgram( ProgramHandle h,
                                                  ProgramParams params,
                                                  Errors& errors)
  {
    const String fileName { params.mFileStem + ".hlsl"};
    TAC_CALL( const String preprocessedShader{
      HLSLPreprocess( "assets/hlsl/" + fileName, errors ) } );

    const DXCCompileParams input
    {
      .mFileName           { fileName },
      .mPreprocessedShader { preprocessedShader },
      .mShaderModel        { sShaderModel },
      .mOutputDir          { RenderApi::GetShaderOutputPath() },
    };

    TAC_CALL( DXCCompileOutput output { DXCCompile( input, errors )  });

    const D3D12ProgramBindings bindings( output.mReflInfo.mReflBindings.data(),
                                         output.mReflInfo.mReflBindings.size() );

    // Here's what im thinking.
    // Every descriptor table has its own root parameter,
    // so there wouldn't be a descriptor table that has CBVs, SRVs, and UAVs.
    // Instead that would be 3 separate root parameters.
#if 0
    for( D3D12_SHADER_INPUT_BIND_DESC& info : output.mReflInfo.mReflBindings )
    {
      ShaderInputToRootSigBinding( info, &bindings );
    }
#endif

    const int programInputCount{ output.mReflInfo.mInputs.size() };
    Vector< DX12Program::Input > programInputs( programInputCount );
    for( int i{ 0 }; i < programInputCount; ++i )
    {
      const DXCReflInfo::Input& reflInput{ output.mReflInfo.mInputs[ i ] };
      programInputs[ i ] = DX12Program::Input
      {
        .mName     { reflInput.mName },
        .mIndex    { reflInput.mIndex },
        .mRegister { reflInput.mRegister },
      };
    }

    mPrograms[ h.GetIndex() ] = DX12Program
    {
      .mFileStem        { params.mFileStem },
      .mVSBlob          { output.mVSBlob },
      .mVSBytecode      { IDxcBlobToBytecode( output.mVSBlob ) },
      .mPSBlob          { output.mPSBlob },
      .mPSBytecode      { IDxcBlobToBytecode( output.mPSBlob ) },
      .mProgramBindings { bindings },
      .mInputs          { programInputs },
    };

  }
} // namespace Tac::Render

