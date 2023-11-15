#include "src/shell/windows/renderer/dx11/shader/tac_dx11_shader_preprocess.h" // self-inc

#include "src/common/string/tac_string.h" // String

namespace Tac::Render
{
  String PreprocessShaderSemanticName( const StringView& line )
  {
    const int iAutoSemantic = line.find( "SV_AUTO_SEMANTIC" );
    if( iAutoSemantic == line.npos )
      return line;

    const int iColon = line.find( ":" );
    if( iColon == line.npos )
      return line;

    int iSemanticCharLast = iColon - 1;
    while( iSemanticCharLast > 0 && IsSpace( line[ iSemanticCharLast ] ) )
      iSemanticCharLast--;

    int iSemanticCharFirst = iSemanticCharLast;
    while( iSemanticCharFirst > 0 && !IsSpace( line[ iSemanticCharFirst - 1 ] ) )
      iSemanticCharFirst--;

    const String newLine
      = line.substr( 0, iAutoSemantic )
      + String( line.data() + iSemanticCharFirst, line.data() + iSemanticCharLast + 1 )
      + ";";

    return newLine;
  }
} // namespace Tac::Render



