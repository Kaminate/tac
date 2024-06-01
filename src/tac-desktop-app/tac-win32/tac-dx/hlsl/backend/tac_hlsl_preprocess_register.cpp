#include "tac_hlsl_preprocess_register.h" // self-inc

#include "tac-std-lib/string/tac_string.h" // String
#include "tac-std-lib/dataprocess/tac_text_parser.h" // ParseData
#include "tac-std-lib/containers/tac_array.h" // Array

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  static int GetRegisterCount( StringView line )
  {
      const int iOpen { line.find( '[' ) };
      const int iClose { line.find( ']' ) };
      if( iOpen == line.npos || iClose == line.npos )
        return 1;
      return ( int )ParseData( line.data() + iOpen + 1,
                               line.data() + iClose ).EatFloat().GetValueUnchecked();
  }

  static char GetResourceLetter( StringView line )
  {
    ParseData parseData( line );
    parseData.EatWhitespace();

    struct ResourceLetter
    {
      const char* mName;
      const char  mLetter;
    };

    const ResourceLetter resourceMap[]
    {
      { "Texture2D", 't'},
      { "Texture3D", 't'},
      { "TextureCube", 't'},
      { "cbuffer", 'b'},
      { "sampler", 's'},
      { "SamplerState", 's'},
    };

    for( auto [ name, letter ] : resourceMap )
      if( parseData.PeekStringExpected( name ) )
        return letter;

    return ( char )0;
  }

  // -----------------------------------------------------------------------------------------------

  // c - The resource type name ( 
  // n - The resources count
  // 
  // ex 1: Texture2D shadowMaps[ 4 ] : TAC_AUTO_REGISTER;
  //       This results in Add( 't', 4 )
  //
  // ex 2: cbuffer CBufferLights : TAC_AUTO_REGISTER;
  //       This results in Add( 'b', 1 )
  // 
  // The return value is the register assigned to each resource.
  int HLSLLinePreprocessorRegister::Add( char c, int n )
  {
    const int iResource { mLetterCounts[ c ] };
    mLetterCounts[ c ] += n;
    return iResource;
  }

  Optional< String > HLSLLinePreprocessorRegister::Preprocess( Input input, Errors& )
  {
    const StringView line{input.mLine};
    const StringView autoRegister { "TAC_AUTO_REGISTER" };
    const int iReplace { line.find( autoRegister ) };
    if( iReplace == line.npos )
      return {};

    const int regCount { GetRegisterCount( line ) };
    const char letter { GetResourceLetter( line ) };
    TAC_ASSERT_MSG( letter, String()
                    + "Cant figure out TAC_AUTO_REGISTER in "
                    + input.mFile + ":" + ToString( input.mLineNumber )
                    + "\n" + input.mLine + "\n"  );

    const int iResource { Add( letter, regCount ) };
    const String registerStr {
      String() + "register(" + ToString( letter ) + ToString( iResource ) + " )" };

    String result;
    result += "// Autogenerated register from ";
    result += __FUNCTION__;
    result += '\n';
    result += line.substr( 0, iReplace ); // everthing before TAC_AUTO_REGISTER
    result += registerStr;
    result += line.substr( iReplace + autoRegister.size() ); // everything after TAC_AUTO_REGISTER
    return result;
  }

} // namespace Tac::Render



