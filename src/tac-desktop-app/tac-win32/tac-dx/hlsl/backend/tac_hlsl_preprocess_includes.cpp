#include "tac_hlsl_preprocess_includes.h" // self-inc

#include "tac-std-lib/string/tac_string.h" // String
#include "tac-std-lib/preprocess/tac_preprocessor.h" // TAC_ASSERT
#include "tac-std-lib/algorithm/tac_algorithm.h" // Contains
#include "tac-std-lib/error/tac_error_handling.h" // TAC_CALL_RET
#include "tac-rhi/render3/tac_render_api.h" // ShaderNameStringView
#include "tac-std-lib/dataprocess/tac_text_parser.h" // ParseData
//#include "tac-rhi/renderer/tac_renderer_backend.h" // GetShaderAssetPath
#include "tac-engine-core/asset/tac_asset.h" // LoadAssetPath

namespace Tac::Render
{
  String             HLSLLinePreprocessorIncludes::IncludeFile( AssetPathStringView path,
                                                                Errors& errors )
  {
    if( IsIncluded( path ) )
      return String();

    mIncluded.push_back( path );

    TAC_CALL_RET( const String includeSource { LoadAssetPath( path, errors ) } );

    String result;
    result += "//===----- (begin include " + path + ") -----===//\n";
    result += includeSource;
    result += "\n//===----- (end include " + path + ") -----===//\n";
    return result;
  }

  bool HLSLLinePreprocessorIncludes::IsIncluded( AssetPathString s ) const
  {
    for( AssetPathStringView includedPath : mIncluded )
      if( ( StringView )includedPath == ( StringView )s )
        return true;
    return false;
  }

  Optional< String > HLSLLinePreprocessorIncludes::Preprocess( Input input, Errors& errors )
  {
    if( !IsIncluded( input.mFile ) )
    {
      mIncluded.push_back( input.mFile );
    }

    const StringView line{ input.mLine };
    ParseData lineParseData( line.data(), line.size() );
    lineParseData.EatWhitespace();

    if( lineParseData.EatStringExpected( "#pragma once" ) )
      return String();

    if( !lineParseData.EatStringExpected( "#include" ) )
      return {};

    lineParseData.EatUntilCharIsPrev( '\"' );
    const char* includeBegin { lineParseData.GetPos() };
    lineParseData.EatUntilCharIsNext( '\"' );
    const char* includeEnd { lineParseData.GetPos() };
    const StringView includeName( includeBegin, includeEnd );
    const AssetPathString assetDir {
      AssetPathStringView( input.mFile ).GetDirectory() };
    const AssetPathString assetPath {
      AssetPathStringView( ( String )assetDir + '/' + includeName ) };

    TAC_ASSERT( Exists( assetPath ) );

    if( IsIncluded( assetPath ) )
      return String();

    String result;
    result += TAC_CALL_RET( IncludeFile( assetPath, errors ) );

    // Including a ".hlsli" file automatically also includes the ".hlsl" file
    if( includeName.ends_with( ".hlsli" ) )
    {
      AssetPathString hlslPath { assetPath };
      hlslPath.replace( ".hlsli", ".hlsl" );

      if( Exists( hlslPath ) && !IsIncluded( hlslPath ) )
        result += TAC_CALL_RET( IncludeFile( hlslPath, errors ) );
    }

    TAC_RAISE_ERROR_IF_RETURN( {},
                               result.empty(),
                               "failed to open include file " + includeName );

    return result;
  }
} // namespace Tac::Render



