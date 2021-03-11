#include "src/common/tacTextParser.h"
#include "src/common/tacString.h"
#include <cstdlib> // strtod
//#include "src/common/tacLocalization.h"
//#include "src/common/tacUtility.h"
//#include "src/common/tacMemory.h"
//#include "src/common/tacFrameMemory.h"
//#include "src/common/tacAlgorithm.h"
//#include "src/common/tacTemporaryMemory.h"

namespace Tac
{

  ParseData::ParseData( const char* bytes, int byteCount )
  {
    mByteCount = byteCount;
    mIByte = 0;
    mBytes = bytes;
  }

  const char*       ParseData::EatByte()
  {
    return EatBytes( 1 );
  }

  const char*       ParseData::EatBytes( const int byteCount )
  {
    const char* bytes = PeekBytes( byteCount );
    mIByte += bytes ? byteCount : 0;
    return bytes;
  }

  StringView        ParseData::EatRestOfLine()
  {
    const char* strBegin = GetPos();
    while( !EatNewLine() && EatByte() ) {}
    return StringView( strBegin, GetPos() );
  }

  bool              ParseData::EatNewLine()
  {
    const int iByte = mIByte;
    mIByte += PeekNewline();
    return iByte != mIByte;
  }

  bool              ParseData::EatWhitespace()
  {
    const int result = PeekWhitespace();
    mIByte += result;
    return result > 0;
  }

  Optional< float > ParseData::EatFloat()
  {
    const StringView stringView = EatWord();
    const String string = stringView; // std::strtod requires a szstring
    char* endptr;
    const double number = std::strtod( string.c_str(), &endptr );
    if( number == 0 && endptr == string.c_str() )
      return {};
    return ( float )number;
  }

  StringView        ParseData::EatWord()
  {
    EatWhitespace();
    const char* stringBegin = GetPos();
    while( !PeekWhitespace() && EatByte() ) {}
    return StringView( stringBegin, GetPos() );
  }

  bool              ParseData::EatUntilCharIsNext( const char c )
  {
    while( const char* next = PeekByte() )
    {
      if( *next == c )
        return true;
      EatByte();
    }
    return false;
  }

  bool              ParseData::EatUntilCharIsPrev( const char c )
  {
    while( GetRemainingByteCount() )
    {
      const char* eaten = EatByte();
      if( !eaten )
        return false;
      if( *eaten == c )
        return true;
    }
    return false;
  }

  bool              ParseData::EatStringExpected( const StringView& str )
  {
    const int iByte = mIByte;
    mIByte += PeekStringExpected( str ) ? str.size() : 0;
    return iByte != mIByte;
  }

  const char*       ParseData::PeekByte() const
  {
    return PeekBytes( 1 );
  }

  const char*       ParseData::PeekBytes( const int byteCount ) const
  {
    return ( byteCount <= GetRemainingByteCount() ) ? GetPos() : nullptr;
  }

  int               ParseData::PeekNewline() const
  {
    if( PeekStringExpected( "\r\n" ) )
      return 2;
    if( PeekStringExpected( "\n" ) )
      return 1;
    return 0;
  }

  char              ParseData::PeekByteUnchecked() const
  {
    return mBytes[ mIByte ];
  }

  int               ParseData::PeekWhitespace() const
  {
    const char* pos = GetPos();
    const int   remainingCharCount = GetRemainingByteCount();
    for( int i = 0; i < remainingCharCount; ++i )
    {
      const char c = pos[ i ];
      if( !IsSpace( c ) )
        return i;
    }
    return remainingCharCount;
  }

  bool              ParseData::PeekStringExpected( const StringView& expected ) const
  {
    const char* actual = PeekBytes( expected.size() );
    const bool result = actual && StringView( actual, expected.size() ) == expected;
    return result;
  }

  const char*       ParseData::GetPos() const
  {
    return mBytes + mIByte;

  }

  int               ParseData::GetRemainingByteCount() const
  {
    return mByteCount - mIByte;
  }
}
