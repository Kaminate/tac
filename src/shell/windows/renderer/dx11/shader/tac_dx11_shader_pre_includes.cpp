#include "src/shell/windows/renderer/dx11/shader/tac_dx11_shader_preprocess.h" // self-inc

#include "src/common/string/tac_string.h" // String
#include "src/common/preprocess/tac_preprocessor.h" // TAC_ASSERT
#include "src/common/algorithm/tac_algorithm.h" // Contains
#include "src/common/error/tac_error_handling.h" // TAC_CALL_RET
#include "src/common/graphics/tac_renderer.h" // ShaderNameStringView
#include "src/common/dataprocess/tac_text_parser.h" // ParseData
#include "src/common/graphics/tac_renderer_backend.h" // GetShaderAssetPath
#include "src/common/assetmanagers/tac_asset.h" // LoadAssetPath

namespace Tac::Render
{

  ShaderPreprocessorIncludes::ShaderPreprocessorIncludes( AssetPathString assetPath)
  {
    mAssetPath = assetPath;
  }
  
  //static ShaderNameStringView GetShaderNameFromIncludeName( const StringView& includeName )
  //{
  //  const int iDot = includeName.find_last_of( '.' );
  //  return iDot == String::npos ? includeName : includeName.substr( 0, iDot );
  //}

  static String IncludeFile(  AssetPathStringView path, Errors& errors )
  {
    String result;
    const String includeSource = TAC_CALL_RET( {}, LoadAssetPath( path, errors ) );
    //const String includeSourcePreproecssed =
    //  TAC_CALL_RET( {}, PreprocessShaderSource( includeSource, errors ));

    result += "//===----- (begin include " + path + ") -----===//\n";
    //result += includeSourcePreproecssed;
    result += includeSource;
    result += "\n//===----- (end include " + path + ") -----===//\n";
    return result;
  }

  Optional<String> ShaderPreprocessorIncludes::Preprocess( const StringView line, Errors& errors )
  {
    ParseData lineParseData( line.data(), line.size() );
    lineParseData.EatWhitespace();

    if( lineParseData.EatStringExpected( "#pragma once" ) )
      return Optional<String>{ "" };

    if( !lineParseData.EatStringExpected( "#include" ) )
      return {};

    lineParseData.EatUntilCharIsPrev( '\"' );
    const char* includeBegin = lineParseData.GetPos();
    lineParseData.EatUntilCharIsNext( '\"' );
    const char* includeEnd = lineParseData.GetPos();
    const StringView includeName( includeBegin, includeEnd );

    {
      if( Contains( mIncluded, includeName ) )
        return Optional<String>{ "" };

      mIncluded.push_back( includeName );
    }

    const AssetPathString assetDir = ( ( AssetPathStringView )mAssetPath ).GetDirectory();
    const AssetPathString assetPath = AssetPathStringView( ( String )assetDir + '/' + includeName );

    String result;
    if( Exists( assetPath ) )
      result += TAC_CALL_RET( {}, IncludeFile( assetPath, errors ) );

    if( includeName.ends_with( ".hlsli" ) )
    {
      AssetPathString hlslPath = assetPath;
      hlslPath.replace( ".hlsli", ".hlsl" );

      if( Exists( hlslPath ) )
        result += TAC_CALL_RET( {}, IncludeFile( hlslPath, errors ) );
    }

    TAC_RAISE_ERROR_IF_RETURN( result.empty(), "failed to open include file " + includeName, {} );

    return result;
  }
} // namespace Tac::Render



