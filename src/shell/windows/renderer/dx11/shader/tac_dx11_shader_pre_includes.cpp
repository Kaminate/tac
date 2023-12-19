#include "src/shell/windows/renderer/dx11/shader/tac_dx11_shader_preprocess.h" // self-inc

#include "src/common/string/tac_string.h" // String
#include "src/common/preprocess/tac_preprocessor.h" // TAC_ASSERT
#include "src/common/error/tac_error_handling.h" // TAC_CALL_RET
#include "src/common/graphics/tac_renderer.h" // ShaderNameStringView
#include "src/common/dataprocess/tac_text_parser.h" // ParseData
#include "src/common/graphics/tac_renderer_backend.h" // GetShaderAssetPath
#include "src/common/assetmanagers/tac_asset.h" // LoadAssetPath

namespace Tac::Render
{

  static ShaderNameStringView GetShaderNameFromIncludeName( const StringView& includeName )
  {
    const int iDot = includeName.find_last_of( '.' );
    return iDot == String::npos ? includeName : includeName.substr( 0, iDot );
  }

  String PreprocessShaderIncludes( const StringView& line, Errors& errors )
  {
    ParseData lineParseData( line.data(), line.size() );
    lineParseData.EatWhitespace();
    if( !lineParseData.EatStringExpected( "#include" ) )
      return line;

    lineParseData.EatUntilCharIsPrev( '\"' );
    const char* includeBegin = lineParseData.GetPos();
    lineParseData.EatUntilCharIsNext( '\"' );
    const char* includeEnd = lineParseData.GetPos();
    const StringView includeName( includeBegin, includeEnd );
    const ShaderNameStringView shaderName = GetShaderNameFromIncludeName( includeName );
    const AssetPathStringView includeAssetPath = GetShaderAssetPath( shaderName );
    const String includeSource = TAC_CALL_RET( {}, LoadAssetPath( includeAssetPath, errors ));
    const String includeSourcePreproecssed =
      TAC_CALL_RET( {}, PreprocessShaderSource( includeSource, errors ));

    String result;
    result += "//===----- (begin include " + includeAssetPath + ") -----===//\n";
    result += includeSourcePreproecssed;
    result += '\n';
    result += "//===----- (end include " + includeAssetPath + ") -----===//\n";
    return result;
  }
} // namespace Tac::Render



