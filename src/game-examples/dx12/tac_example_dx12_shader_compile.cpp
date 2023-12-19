#include "tac_example_dx12_shader_compile.h" // self-inc
#include "tac_example_dx12_2_dxc.h"

#include "src/common/error/tac_error_handling.h"
#include "src/common/assetmanagers/tac_asset.h"
#include "src/common/dataprocess/tac_text_parser.h"
#include "src/common/shell/tac_shell.h" // sShellPrefPath
#include "src/shell/windows/renderer/dx11/shader/tac_dx11_shader_preprocess.h"


namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  // static helper functions

  static String DX12PreprocessShader( StringView shader )
  {
    String newShader;

    ParseData parse( shader );
    while( parse )
    {
      const StringView line = parse.EatRestOfLine();

      newShader += PreprocessShaderSemanticName( line );
      newShader += '\n';
    }

    return newShader;
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

  // DX12ProgramCompiler::Result definition
  
  D3D12_SHADER_BYTECODE DX12ProgramCompiler::Result::GetBytecode( ShaderType type )
  {
    auto blob = ( IDxcBlob* )mBlobs[ ( int )type ];
    return blob ?
      D3D12_SHADER_BYTECODE{ blob->GetBufferPointer(), blob->GetBufferSize() } :
      D3D12_SHADER_BYTECODE{};
  }

  // -----------------------------------------------------------------------------------------------

  // DX12ProgramCompiler definitions

  const D3D_SHADER_MODEL shaderModel = D3D_SHADER_MODEL_6_5;

  DX12ProgramCompiler::DX12ProgramCompiler( ID3D12Device* device, Errors& errors )
  {
    const D3D_SHADER_MODEL highestShaderModel = GetHighestShaderModel( device );
    TAC_RAISE_ERROR_IF( shaderModel > highestShaderModel, "Shader model too high" ); 
  }

  static PCom<IDxcBlob> CompileShader( ShaderType shaderType,
                                       AssetPathStringView shaderAssetPath,
                                       Errors& errors)
  {
    const String shaderStrRaw = TAC_CALL_RET( {}, LoadAssetPath( shaderAssetPath, errors ));
    const String shaderStrProcessed = DX12PreprocessShader( shaderStrRaw );

    const char* entryPoints[ ( int )ShaderType::Count ]{};
    entryPoints[ ( int )ShaderType::Vertex ] = "VSMain";
    entryPoints[ ( int )ShaderType::Fragment ] = "PSMain";

    const char* entryPoint = entryPoints[ (int)shaderType ];
    if( !entryPoint )
      return {};

    if( !shaderStrProcessed.contains( String() + entryPoint + "(" ) )
      return {};

    const DXC::Input input
    {
      .mShaderAssetPath = shaderAssetPath,
      .mPreprocessedShader = shaderStrProcessed,
      .mEntryPoint = entryPoint,
      .mType = shaderType,
      .mShaderModel = shaderModel,
      .mOutputDir = sShellPrefPath,
    };
    return TAC_CALL_RET( {}, DXC::Compile( input, errors ));
  }

  // you know... this code assumes all the shaders are in the same file... why would that
  // even be a fair assumption...
  DX12ProgramCompiler::Result DX12ProgramCompiler::Compile( AssetPathStringView shaderAssetPath,
                                                            Errors& errors )
  {
    Result result;
    for( int i = 0; i < ( int )ShaderType::Count; ++i )
    {
      result.mBlobs[ i ] = TAC_CALL_RET( {},
                                         CompileShader( ( ShaderType )i,
                                         shaderAssetPath,
                                         errors ) );
    }

    return result;
  }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac::Render

