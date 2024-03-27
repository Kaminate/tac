#include "tac_hlsl_preprocess.h" // self-inc

#include "tac-win32/dx/hlsl/backend/
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac::Render
{

  String DX12PreprocessShader( AssetPathStringView assetPath, Errors& errors )
  {
    const String shaderStrRaw = TAC_CALL_RET( {}, LoadAssetPath( assetPath, errors ));

    ShaderPreprocessorRegister processReg;
    ShaderPreprocessorIncludes processInc( assetPath );
    ShaderPreprocessorPadding processPad;

    ShaderPreprocessor preprocessor;
    preprocessor.mAssetPath = assetPath;
    preprocessor.Add( PreprocessShaderLineComment );
    preprocessor.Add( PreprocessShaderLineBitfield );
    preprocessor.Add( PreprocessShaderLineDeprecations );
    preprocessor.Add( PreprocessShaderLineSemanticName );
    preprocessor.Add( &processReg );
    preprocessor.Add( &processInc );
    preprocessor.Add( &processPad );

    return DXPreprocessShaderSource( shaderStrRaw, &preprocessor, errors );
  }
} // namespace Tac::Render

