#include "tac_text_parser.h" // self-inc

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{

  ParseData::ParseData( StringView str ) : mStr( str ) { }

  ParseData::ParseData( const char* bytes ) : mStr ( bytes ) { }

  ParseData::ParseData( const char* bytes, int byteCount ) : mStr ( bytes, byteCount ) { }

  ParseData::ParseData( const char* begin, const char* end ) : mStr( begin, end ) { }

  auto ParseData::EatByte() -> const char*
  {
    return EatBytes( 1 );
  }

  auto ParseData::EatBytes( const int byteCount ) -> const char*
  {
    const char* bytes { PeekBytes( byteCount ) };
    mIByte += bytes ? byteCount : 0;
    return bytes;
  }

  auto ParseData::EatRestOfLine() -> StringView
  {
    const char* strBegin { GetPos() };
    do
    {
      if( PeekNewline() )
      {
        const char* strEnd { GetPos() };
        EatNewLine();
        // The \r\n will be eaten but not returned
        return StringView( strBegin, strEnd );
      }
    } while( EatByte() );

    // Have reached EOF
    return StringView( strBegin, GetPos() );
  }

  bool ParseData::EatNewLine()
  {
    const int iByte { mIByte };
    mIByte += PeekNewline();
    return iByte != mIByte;
  }

  bool ParseData::EatWhitespace()
  {
    const int result { PeekWhitespace() };
    mIByte += result;
    return result > 0;
  }

  auto ParseData::EatFloat() -> Optional<float>
  {
    EatWhitespace();
    const char* strBegin { GetPos() };
    while( const char* next{ PeekByte() } )
    {
      const bool valid { StringView( "0123456789.e-" ).find_first_of( *next ) != String::npos };
      if( !valid )
        break;

      EatByte();
    }

    const StringView sv( strBegin, GetPos() );
    return Atof( sv );
  }

  auto ParseData::EatFloat(Errors& errors) -> float
  {
    Optional< float > f { EatFloat() };
    TAC_RAISE_ERROR_IF_RETURN( !f.HasValue(), "Failed to eat float" );
    return f;
  }

  auto ParseData::EatWord() -> StringView
  {
    EatWhitespace();
    const char* stringBegin { GetPos() };
    while( !PeekWhitespace() && EatByte() ) {}
    return StringView( stringBegin, GetPos() );
  }

  bool ParseData::EatUntilCharIsNext( const char c )
  {
    while( const char* next{ PeekByte() } )
    {
      if( *next == c )
        return true;
      EatByte();
    }
    return false;
  }

  bool ParseData::EatUntilCharIsPrev( const char c )
  {
    while( GetRemainingByteCount() )
    {
      const char* eaten { EatByte() };
      if( !eaten )
        return false;

      if( *eaten == c )
        return true;
    }
    return false;
  }

  bool ParseData::EatStringExpected( const StringView str )
  {
    const int iByte { mIByte };
    mIByte += PeekStringExpected( str ) ? str.size() : 0;
    return iByte != mIByte;
  }

  auto ParseData::PeekByte()  const -> const char*
  {
    return PeekBytes( 1 );
  }

  auto ParseData::PeekBytes( const int byteCount ) const -> const char*
  {
    return ( byteCount <= GetRemainingByteCount() ) ? GetPos() : nullptr;
  }

  auto ParseData::PeekNewline() const -> int
  {
    if( PeekStringExpected( "\r\n" ) )
      return 2;
    if( PeekStringExpected( "\n" ) )
      return 1;
    return 0;
  }

  char ParseData::PeekByteUnchecked() const
  {
    return mStr[ mIByte ];
  }

  // returns the number of bytes between the current position and the next non-whitespace character
  //
  // this return value is useful to skip over whitespace
  auto ParseData::PeekWhitespace() const -> int
  {
    const char* pos { GetPos() };
    const int   remainingCharCount { GetRemainingByteCount() };
    for( int i{}; i < remainingCharCount; ++i )
    {
      const char c { pos[ i ] };
      if( !IsSpace( c ) )
        return i;
    }
    return remainingCharCount;
  }

  bool ParseData::PeekStringExpected( const StringView expected ) const
  {
    const char* actual { PeekBytes( expected.size() ) };
    const bool result { actual && StringView( actual, expected.size() ) == expected };
    return result;
  }

  auto ParseData::GetPos() const -> const char*
  {
    return mStr.data() + mIByte;

  }

  auto ParseData::GetRemainingByteCount() const -> int { return mStr.size() - mIByte; }

  ParseData::operator bool() const
  {
    return GetRemainingByteCount() > 0;
  }
} // namespace Tac
