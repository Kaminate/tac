#include "tac_dx_shader_preprocess.h" // self-inc

//#include "src/common/assetmanagers/tac_asset.h"
//#include "src/common/containers/tac_array.h"
#include "src/common/error/tac_error_handling.h"
#include "src/common/dataprocess/tac_text_parser.h"
//#include "src/common/graphics/tac_renderer.h"
//#include "src/common/graphics/tac_renderer_backend.h"
//#include "src/common/math/tac_math.h"
//#include "src/common/memory/tac_frame_memory.h"

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------
  void ShaderPreprocessor::Add( IShaderLinePreprocessor* p )
  {
    mProcessors.push_back( p );
  }

  void ShaderPreprocessor::Add( ShaderPreprocessFn fn )
  {
    mFns.push_back( fn );
  }

  //Span< IShaderLinePreprocessor* > ShaderPreprocessor::GetProcessors()
  //{
  //  return { mProcessors.begin(), mProcessors.size() };
  //}

  // -----------------------------------------------------------------------------------------------

  //IShaderLinePreprocessors::~IShaderLinePreprocessors()
  //{
  //  for( auto processor : mProcessors )
  //  {
  //    TAC_DELETE processor;
  //  }
  //}

  //Optional<String> IShaderLinePreprocessors::Preprocess( const StringView sv, Errors& errors )
  //{
  //  for( auto processor : mProcessors )
  //  {
  //    auto opt = TAC_CALL_RET( {}, processor->Preprocess( sv, errors ) );
  //    if( opt.HasValue() )
  //      return opt;
  //  }
  //  return {};
  //};

  // -----------------------------------------------------------------------------------------------

  Optional<String> ShaderPreprocessor::PreprocessLine( const StringView line, Errors& errors)
  {
    for( auto lineProcessor : mProcessors )
    {
      Optional< String > optNewLines = TAC_CALL_RET( {}, lineProcessor->Preprocess( line, errors ) );
      if( optNewLines.HasValue() )
        return optNewLines;
    }

    for( auto fn : mFns )
    {
      Optional< String > optNewLines = TAC_CALL_RET( {}, fn( line, errors ) );
      if( optNewLines.HasValue() )
        return optNewLines;
    }

    return {};
  }


  String DXPreprocessShaderSource( const StringView sourceCode,
                                   ShaderPreprocessor* preprocessor,
                                   Errors& errors )
  {
    String result;

    ParseData shaderParseData( sourceCode.data(), sourceCode.size() );
    while( shaderParseData.GetRemainingByteCount() )
    {
      const StringView oldLine = shaderParseData.EatRestOfLine();

      Optional< String > optNewLines = TAC_CALL_RET(
        {}, preprocessor->PreprocessLine( oldLine, errors ) );

      if( optNewLines.HasValue() )
      {
        String newLines = optNewLines.GetValueUnchecked();
        if( newLines != oldLine )
        {
          // Recurse
          optNewLines = TAC_CALL_RET(
            {}, DXPreprocessShaderSource( newLines, preprocessor, errors ) );
        }
      }

      result += optNewLines.GetValueOr( oldLine );
      result += '\n';
    }

    return result;
  }

}// namespace Tac::Render