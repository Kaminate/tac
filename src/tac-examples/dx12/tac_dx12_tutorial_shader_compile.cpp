#include "tac_dx12_tutorial_shader_compile.h" // self-inc

//#include "tac_dx12_tutorial_2_dxc.h"
#include "tac-dx/dxc/tac_dxc.h"

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-std-lib/dataprocess/tac_text_parser.h"
#include "tac-std-lib/os/tac_os.h" //tmp
//#include "tac-std-lib/shell/tac_shell.h" // sShellPrefPath
//#include "tac-dx/dx12/tac_dx12_shader_preprocess.h"
#include "tac-dx/hlsl/tac_hlsl_preprocess.h"


namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------
  static       FileSys::Path    sOutputDir;
  static const D3D_SHADER_MODEL kShaderModel{ D3D_SHADER_MODEL_6_5 };

  static auto GetHighestShaderModel( ID3D12Device* device ) -> D3D_SHADER_MODEL
  {
    const D3D_SHADER_MODEL lowestDefined{ D3D_SHADER_MODEL_5_1 };
    const D3D_SHADER_MODEL highestDefined{ D3D_SHADER_MODEL_6_7 }; // D3D_HIGHEST_SHADER_MODEL undefined?;
    for( D3D_SHADER_MODEL shaderModel{ highestDefined };
         shaderModel >= lowestDefined;
         shaderModel = D3D_SHADER_MODEL( shaderModel - 1 ) )
    {
      // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_feature_data_shader_model
      //   After the function completes successfully, the HighestShaderModel field contains the
      //   highest shader model that is both supported by the device and no higher than the
      //   shader model passed in.
      dynmc D3D12_FEATURE_DATA_SHADER_MODEL featureData{ shaderModel };
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

  DX12ExampleProgramCompiler::DX12ExampleProgramCompiler( Params params, Errors& errors )
  {
    sOutputDir = params.mOutputDir;
    const D3D_SHADER_MODEL highestShaderModel{ GetHighestShaderModel( params.mDevice ) };
    TAC_RAISE_ERROR_IF( kShaderModel > highestShaderModel, "Shader model too high" );
  }

  // you know... this code assumes all the shaders are in the same file... why would that
  // even be a fair assumption...
  auto DX12ExampleProgramCompiler::Compile( const AssetPathStringView& path, Errors& errors ) const -> Result
  {
    const String shaderStrProcessed { HLSLPreprocessor::Process( { path }, errors ) };
    TAC_CALL_RET( DXCCompileOutput compileOutput{ DXCCompile(
      DXCCompileParams
      {
        .mFileName           { path.GetFilename() },
        .mOutputDir          { sOutputDir },
        .mPreprocessedShader { shaderStrProcessed },
        .mShaderModel        { kShaderModel },
      }, errors ) } );
    return Result
    {
      .mVSBlob { compileOutput.mVSBlob },
      .mPSBlob { compileOutput.mPSBlob },
      .mVSBytecode { compileOutput.mVSBlob->GetBufferPointer(), compileOutput.mVSBlob->GetBufferSize() },
      .mPSBytecode { compileOutput.mPSBlob->GetBufferPointer(), compileOutput.mPSBlob->GetBufferSize() },
    };
  }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac::Render

