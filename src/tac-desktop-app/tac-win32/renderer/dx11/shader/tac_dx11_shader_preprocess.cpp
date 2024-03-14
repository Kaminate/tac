#include "tac_dx11_shader_preprocess.h" // self-inc

#include "tac-win32/renderer/dx_shader_preprocess/tac_dx_shader_preprocess.h"
#include "tac-std-lib/filesystem/tac_asset.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/dataprocess/tac_text_parser.h"
#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-rhi/renderer/tac_renderer_backend.h"
#include "tac-std-lib/math/tac_math.h"
//#include "tac-engine-core/framememory/tac_frame_memory.h"

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


  String DX11PreprocessShader( const AssetPathStringView& assetPath, Errors& errors )
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



