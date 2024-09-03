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

  Optional<String> HLSLFilePreprocessor::PreprocessLine( HLSLLinePreprocessor::Input input,
                                                         Errors& errors)
  {
    for( auto lineProcessor : mProcessors )
    {
      TAC_CALL_RET( {},
                    Optional< String > optNewLines{ lineProcessor->Preprocess( input, errors ) } );
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
    int lineNumber{};
    while( shaderParseData.GetRemainingByteCount() )
    {
      lineNumber++;
      const StringView origLine { shaderParseData.EatRestOfLine() };
      const HLSLLinePreprocessor::Input preprocessorInput
      {
        .mLine       { origLine },
        .mFile       { mAssetPath },
        .mLineNumber { lineNumber },
      };
      TAC_CALL_RET( {}, Optional< String > optResult{
        PreprocessLine( preprocessorInput, errors ) } );
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
    mAssetPath = assetPath;
    TAC_CALL_RET( {}, const String sourceCode{ LoadAssetPath( assetPath, errors ) } );
    return PreprocessSource( sourceCode, errors );
  }

}// namespace Tac::Render
