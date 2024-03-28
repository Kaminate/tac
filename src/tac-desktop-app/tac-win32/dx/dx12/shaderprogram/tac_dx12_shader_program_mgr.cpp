#include "tac_dx12_shader_program_mgr.h" // self-inc

#include "tac-win32/dx/hlsl/tac_hlsl_preprocess.h"
#include "tac-win32/dx/dxc/tac_dxc.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"

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
    const D3D_SHADER_MODEL lowestDefined = D3D_SHADER_MODEL_5_1;
    const D3D_SHADER_MODEL highestDefined = D3D_SHADER_MODEL_6_7; // D3D_HIGHEST_SHADER_MODEL undefined?;
    for( D3D_SHADER_MODEL shaderModel = highestDefined; 
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
        sizeof(D3D12_FEATURE_DATA_SHADER_MODEL) ) ) )
        
        // For some godforsaken fucking reason, this isn't the same as shaderModel
        return featureData.HighestShaderModel;
    }

    return lowestDefined;
  }

  // -----------------------------------------------------------------------------------------------

  void DX12ShaderProgramMgr::Init( ID3D12Device* device, Errors& errors )
  {
    mDevice = device;

    const D3D_SHADER_MODEL highestShaderModel = GetHighestShaderModel( device );
    TAC_RAISE_ERROR_IF( sShaderModel > highestShaderModel, "Shader model too high" );
  }

  DX12ShaderProgram* DX12ShaderProgramMgr::FindProgram( ProgramHandle h )
  {
    return h.IsValid() ? &mShaderPrograms[ h.GetIndex() ] : nullptr;
  }

  void DX12ShaderProgramMgr::CreateShaderProgram( ProgramHandle h,
                                                  ShaderProgramParams params,
                                                  Errors& errors)
  {
    const String fileName = params.mFileStem + ".hlsl";
    const String path = "assets/hlsl/" + fileName;
    const String preprocessedShader = TAC_CALL( HLSLPreprocess( path, errors ) );
    const Filesystem::Path outputDir = RenderApi::GetShaderOutputPath();
    const DXCCompileParams input
    {
      .mFileName = fileName,
      .mPreprocessedShader = preprocessedShader,
      .mShaderModel = sShaderModel,
      .mOutputDir = outputDir,
    };

    DXCCompileOutput output = TAC_CALL( DXCCompile( input, errors ) );

    mShaderPrograms[ h.GetIndex() ] = DX12ShaderProgram
    {
      .mFileStem = params.mFileStem,
      .mVSBlob = output.mVSBlob,
      .mVSBytecode = IDxcBlobToBytecode( output.mVSBlob ),
      .mPSBlob = output.mPSBlob,
      .mPSBytecode = IDxcBlobToBytecode( output.mPSBlob ),
    };

  }
} // namespace Tac::Render

