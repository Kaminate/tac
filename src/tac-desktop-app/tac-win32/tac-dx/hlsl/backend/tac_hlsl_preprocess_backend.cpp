#include "tac_hlsl_preprocess_backend.h" // self-inc

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/dataprocess/tac_text_parser.h"

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  void HLSLFilePreprocessor::Add( HLSLLinePreprocessor* p )
  {
    mProcessors.push_back( p );
    //p->mParent = this;
  }

  // -----------------------------------------------------------------------------------------------

  Optional<String> HLSLFilePreprocessor::PreprocessLine( const StringView line, Errors& errors)
  {
    for( auto lineProcessor : mProcessors )
    {
      TAC_CALL_RET( {},
                    Optional< String > optNewLines{ lineProcessor->Preprocess( line, errors ) } );
      if( optNewLines.HasValue() )
      {
        return optNewLines;
      }
    }

    return {};
  }

  String HLSLFilePreprocessor::PreprocessSource( StringView sourceCode, Errors& errors )
  {
    String result;

    ParseData shaderParseData( sourceCode.data(), sourceCode.size() );
    while( shaderParseData.GetRemainingByteCount() )
    {
      const StringView origLine { shaderParseData.EatRestOfLine() };
      TAC_CALL_RET( {}, Optional< String > optResult{ PreprocessLine( origLine, errors ) } );
      String procLine { optResult.GetValueOr( origLine ) };
      String recursed;
      const bool isSame { ( StringView )procLine == origLine };
      if( !isSame )
      {
        // Recurse
        recursed = TAC_CALL_RET( {}, PreprocessSource( procLine, errors ) );
        procLine = recursed;
      }

      result += procLine;
      result += '\n';
    }

    return result;
  }

  String HLSLFilePreprocessor::PreprocessFile( const AssetPathStringView assetPath,
                                               Errors& errors )
  {
    TAC_CALL_RET( {}, const String sourceCode{ LoadAssetPath( assetPath, errors ) } );
    return PreprocessSource( sourceCode, errors );

  }

}// namespace Tac::Render