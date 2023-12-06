#include "src/shell/windows/renderer/dx11/shader/tac_dx11_shader_preprocess.h" // self-inc

#include "src/common/string/tac_string.h" // String

namespace Tac::Render
{
  String PreprocessShaderSemanticName( const StringView& line )
  {
    const StringView autoSemantic = "TAC_AUTO_SEMANTIC";
    const int iAutoSemantic = line.find( autoSemantic );
    if( iAutoSemantic == line.npos )
      return line;

    int i = line.find( ":" );
    TAC_ASSERT( i != String::npos );
    i--;
    while( IsSpace( line[ i ] ) )
      i--;

    const char* varEnd = line.data() + i + 1;
    while( !IsSpace( line[i - 1] ) )
      i--;

    const char* varBegin = line.data() + i;
    TAC_ASSERT( varEnd > varBegin );

    const StringView varName( varBegin, varEnd );

    String result = line;
    result.replace( autoSemantic, varName );
    return result;
  }
} // namespace Tac::Render



