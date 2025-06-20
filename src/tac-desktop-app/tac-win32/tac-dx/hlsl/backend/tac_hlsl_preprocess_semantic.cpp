#include "tac_hlsl_preprocess_semantic.h" // self-inc

#include "tac-std-lib/string/tac_string.h" // String

namespace Tac::Render
{
  Optional< String > HLSLLinePreprocessorSemantic::Preprocess( Input input, Errors& )
  {
    const StringView line{input.mLine};
    if( !line.contains( "TAC_AUTO_SEMANTIC" ) )
      return {};

    const StringView autoSemantic { "TAC_AUTO_SEMANTIC" };
    //const int iAutoSemantic { line.find( autoSemantic ) };

    int i { line.find( ":" ) };
    TAC_ASSERT( i != String::npos );
    i--;
    while( IsSpace( line[ i ] ) )
      i--;

    const char* varEnd { line.data() + i + 1 };
    while( !IsSpace( line[i - 1] ) )
      i--;

    const char* varBegin { line.data() + i };
    TAC_ASSERT( varEnd > varBegin );

    const StringView varName( varBegin, varEnd );

    String result { line };
    result.replace( autoSemantic, varName );
    return result;
  }
} // namespace Tac::Render



