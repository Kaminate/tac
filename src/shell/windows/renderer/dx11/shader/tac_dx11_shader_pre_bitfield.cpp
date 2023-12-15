// For example, if you see the following in shader code,
//
//   TAC_DEFINE_BITFIELD_BEGIN;
//   TAC_DEFINE_BITFIELD_ELEMENT( LightGetType, 4 );
//   TAC_DEFINE_BITFIELD_ELEMENT( LightGetCastsShadows, 1 );
//   TAC_DEFINE_BITFIELD_END;
//
//   void Foo()
//   {
//     uint lightType = LightGetType( light.mFlags );
//     uint castsShadows = LightGetCastsShadows( light.mFlags );
//     ...
//   }
//
// This defines the following functions:
//
//   uint LightGetType( uint flags )
//   {
//      return ( flags >> 0 ) & 0x2;
//   }
//
//   uint LightGetCastsShadows( uint flags )
//   {
//      return ( flags >> 2 ) & 0x1;
//   }
//
//
// see Tac::ShaderFlags
//
//   TAC_DEFINE_BITFIELD_ELEMENT( FnName, BitCount );
//
// becomes
//
//   uint LightGetCastsShadows( uint flags )
//   {
//      return ( flags >> prevBitCount ) & ( ( 1 << bitCount ) - 1 );
//   }

#include "src/shell/windows/renderer/dx11/shader/tac_dx11_shader_preprocess.h" // self-inc

#include "src/common/string/tac_string.h" // String
#include "src/common/preprocess/tac_preprocessor.h" // TAC_ASSERT

namespace Tac::Render
{
  static String GetFunctionName( const StringView& line )
  {
    int iFunctionNameBegin = line.find( '(' ) + 1;
    while( IsSpace( line[ iFunctionNameBegin ] ) )
      iFunctionNameBegin++;

    int iFunctionNameEnd = line.find( ',' );
    return String( line.data() + iFunctionNameBegin,
                   line.data() + iFunctionNameEnd );
  }

  static int GetBitCount( const StringView& line )
  {
    if constexpr( IsDebugMode )
    {
      int digitCount = 0;
      for( char c : line )
        if( IsDigit( c ) )
          digitCount++;
      TAC_ASSERT( digitCount == 1 );
    }

    const int n = line.size();
    for( int i = 0; i < n; ++i )
    {
      const char c = line[ n - i - 1 ];
      if( IsDigit( c ) )
      {
        return c - '0';
      }
    }

    TAC_ASSERT_INVALID_CODE_PATH;
    return 0;
  }

  static String GetLeadingWhitespace( const StringView& line )
  {
    String result;
    for( char c : line )
    {
      if( !IsSpace( c ) )
        break;
      result += c;
    }
    return result;
  }

  String PreprocessShaderBitfield( const StringView& line )
  {
    static int sRunningBitCount;
    static bool sProcessing;

    if( line.contains( "TAC_DEFINE_BITFIELD_BEGIN" ) )
    {
      TAC_ASSERT( !sProcessing );
      sProcessing = true;
      return "//   Bitfield getter functions auto generated by " + String( __FUNCTION__ );
    }

    if( line.contains( "TAC_DEFINE_BITFIELD_ELEMENT" ) )
    {
      TAC_ASSERT( sProcessing );

      const int spacesPerTab = 2;
      const String tab( spacesPerTab, ' ' );
      const int shift = sRunningBitCount;

      const String functionName = GetFunctionName( line );
      const int bitCount = GetBitCount( line );
      const String space = GetLeadingWhitespace( line );

      const int mask = ( 1 << bitCount ) - 1;

      const String bitCountStr = String( 1, '0' + ( char )bitCount );
      const String maskStr = ToString( mask );

      const String shifted
        = shift
        ? "( flags >> " + ToString( shift ) + " )"
        : String( "flags" );

      sRunningBitCount += bitCount;

      String result;
      result += space + result + "uint " + functionName + "( uint flags )\n";
      result += space + "{\n";
      result += space + tab + "return " + shifted + " & " + maskStr + ";\n";
      result += space + "}\n";
      return result;
    }

    if( line.contains( "TAC_DEFINE_BITFIELD_END" ) )
    {
      TAC_ASSERT( sProcessing );
      sProcessing = false;
      sRunningBitCount = 0;
      return "";
    }

    return line;
  }
} // namespace Tac::Render



