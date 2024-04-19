#include "tac_example_dx12_shader_compile.h" // self-inc

//#include "tac_example_dx12_2_dxc.h"
#include "tac-win32/dx/dxc/tac_dxc.h"

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/filesystem/tac_asset.h"
#include "tac-std-lib/dataprocess/tac_text_parser.h"
#include "tac-std-lib/os/tac_os.h" //tmp
//#include "tac-std-lib/shell/tac_shell.h" // sShellPrefPath
//#include "tac-win32/dx/dx12/tac_dx12_shader_preprocess.h"
#include "tac-win32/dx/hlsl/tac_hlsl_preprocess.h"


namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------
  static Filesystem::Path sOutputDir; // = sShellPrefPath,

  // static helper functions


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

  // DX12ExampleProgramCompiler::Result definition
  
#if 0
  D3D12_SHADER_BYTECODE DX12ExampleProgramCompiler::Result::GetBytecode( ShaderType type )
  {
    auto blob = ( IDxcBlob* )mBlobs[ ( int )type ];
    return blob ?
      D3D12_SHADER_BYTECODE{ blob->GetBufferPointer(), blob->GetBufferSize() } :
      D3D12_SHADER_BYTECODE{};
  }
#endif

  // -----------------------------------------------------------------------------------------------

  // DX12ExampleProgramCompiler definitions

  const D3D_SHADER_MODEL shaderModel = D3D_SHADER_MODEL_6_5;

  DX12ExampleProgramCompiler::DX12ExampleProgramCompiler( Params params, Errors& errors )
  {
    ID3D12Device* device = params.mDevice;
    sOutputDir = params.mOutputDir;
    const D3D_SHADER_MODEL highestShaderModel = GetHighestShaderModel( device );
    TAC_RAISE_ERROR_IF( shaderModel > highestShaderModel, "Shader model too high" );
  }

  // you know... this code assumes all the shaders are in the same file... why would that
  // even be a fair assumption...
  DX12ExampleProgramCompiler::Result DX12ExampleProgramCompiler::Compile( const AssetPathStringView& path,
                                                                          Errors& errors )
  {

    const String shaderStrProcessed = HLSLPreprocess( path, errors );

    DXCCompileParams compileParams
    {
      .mFileName = path,
      .mPreprocessedShader = shaderStrProcessed,
      .mShaderModel = shaderModel,
      .mOutputDir = sOutputDir,
    };

    DXCCompileOutput compileOutput = TAC_CALL_RET( {}, DXCCompile( compileParams, errors ) );

    PCom< IDxcBlob >& PSBlob = compileOutput.mPSBlob;
    PCom< IDxcBlob >& VSBlob = compileOutput.mVSBlob;
    D3D12_SHADER_BYTECODE VSBytecode{ VSBlob->GetBufferPointer(), VSBlob->GetBufferSize() };
    D3D12_SHADER_BYTECODE PSBytecode{ PSBlob->GetBufferPointer(),  PSBlob->GetBufferSize() };

    return Result
    {
      .mVSBlob = VSBlob,
      .mPSBlob = PSBlob,
      .mVSBytecode = VSBytecode,
      .mPSBytecode = PSBytecode,
    };
  }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac::Render

