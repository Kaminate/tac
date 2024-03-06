#include "src/shell/windows/renderer/dx11/shader/tac_dx11_shader_preprocess.h" // self-inc

#include "src/shell/windows/renderer/dx_shader_preprocess/tac_dx_shader_preprocess.h"
#include "src/common/assetmanagers/tac_asset.h"
#include "src/common/containers/tac_array.h"
#include "src/common/error/tac_error_handling.h"
#include "src/common/dataprocess/tac_text_parser.h"
#include "src/common/graphics/renderer/tac_renderer.h"
#include "src/common/graphics/renderer/tac_renderer_backend.h"
#include "src/common/math/tac_math.h"
#include "src/common/memory/tac_frame_memory.h"

namespace Tac::Render
{
  bool IsSingleLineCommented( const StringView& line )
  {
    ParseData shaderParseData( line.data(), line.size() );
    shaderParseData.EatWhitespace();
    return shaderParseData.EatStringExpected( "//" );
  }

  Optional<String> PreprocessShaderLineComment( const StringView line, Errors& )
  {
    return IsSingleLineCommented( line ) ? Optional< String >( line ) : Optional< String >{};
  }

  // -----------------------------------------------------------------------------------------------
  Optional<String> ShaderPreprocessorFunction::Preprocess( const StringView sv, Errors& errors ) 
  {
    return mFn(  sv, errors );
  }

  ShaderPreprocessorFunction::ShaderPreprocessorFunction( ShaderPreprocessFn fn ) : mFn( fn ) {}
  // -----------------------------------------------------------------------------------------------


  String DX11PreprocessShader( AssetPathStringView assetPath, Errors& errors )
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



