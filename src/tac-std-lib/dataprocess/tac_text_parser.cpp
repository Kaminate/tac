#include "tac_text_parser.h" // self-inc

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/error/tac_error_handling.h"

//import std; // cstdlib(strtof)

namespace Tac
{

  ParseData::ParseData( StringView str ) : mStr( str ) { }

  ParseData::ParseData( const char* bytes ) : mStr ( bytes ) { }

  ParseData::ParseData( const char* bytes, int byteCount ) : mStr ( bytes, byteCount ) { }

  ParseData::ParseData( const char* begin, const char* end ) : mStr( begin, end ) { }

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
    do
    {
      if( PeekNewline() )
      {
        const char* strEnd = GetPos();
        EatNewLine();
        // The \r\n will be eaten but not returned
        return StringView( strBegin, strEnd );
      }
    } while( EatByte() );

    // Have reached EOF
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

  static Optional<float> ToFloat( StringView s )
  {
    return Atof( s );
#if 0
    // std::strtof requires a szstring
    const String szstr( s );
    const char* szbegin = szstr.data();
    char* szend;
    const float number = std::strtof( szbegin, &szend );
    const bool valid = !( number == 0 && szend == szbegin );
    return valid ? Optional<float>{ number } : Optional<float>{};
#endif
  }

  Optional<float>   ParseData::EatFloat()
  {
    EatWhitespace();
    const char* strBegin = GetPos();
    while( const char* next = PeekByte() )
    {
      const bool valid = StringView( "0123456789.e-" ).find_first_of( *next ) != String::npos;
      if( !valid )
        break;

      EatByte();
    }

    const StringView sv( strBegin, GetPos() );
    return ToFloat( sv );
  }

  float ParseData::EatFloat(Errors& errors)
  {
    Optional<float> f = EatFloat();
    TAC_RAISE_ERROR_IF_RETURN( !f.HasValue(), "Failed to eat float", {} );
    return f;
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
    return mStr[ mIByte ];
  }

  // returns the number of bytes between the current position and the next non-whitespace character
  //
  // this return value is useful to skip over whitespace
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
    return mStr.data() + mIByte;

  }

  int               ParseData::GetRemainingByteCount() const
  {
    //return mByteCount - mIByte;
    return mStr.size() - mIByte;
  }

  ParseData::operator bool() const
  {
    return GetRemainingByteCount() > 0;
  }
} // namespace Tac
