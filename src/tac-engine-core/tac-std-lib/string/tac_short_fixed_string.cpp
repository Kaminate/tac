#include "tac_short_fixed_string.h" // self-inc

#include "tac-std-lib/string/tac_string_view.h"

namespace Tac
{

  static void FixedStringAppend( const ShortFixedString::Data& data, const StringView sv )
  {
    const int oldSize { *data.mSize };
    const int maxSize { data.mCapacity - 1 };
    if( oldSize == maxSize )
      return;

    dynmc int n{ sv.size() };
    if( oldSize + n > maxSize )
      n = maxSize - oldSize;

    const char* src { sv.data() };
    MemCpy( data.mBuf + oldSize, src, n );
    const StringView dots{ "..." };
    if( n != sv.size() )
      MemCpy( data.mBuf + maxSize - dots.size(), dots, dots.size() );

    const int newSize{ *data.mSize + n };
    *data.mSize = newSize;
    data.mBuf[ newSize ] = '\0';
  }

  static void FixedStringAssign( const ShortFixedString::Data& data, const StringView sv )
  {
    *data.mSize = 0;
    FixedStringAppend( data, sv );
  }


  ShortFixedString::ShortFixedString( const char* s )        { assign( s ); }
  ShortFixedString::ShortFixedString( const char* s, int n ) { assign( StringView( s, n ) ); }
  ShortFixedString::ShortFixedString( StringView sv ) { assign( sv ); }

  auto ShortFixedString::Concat( Arg s0, Arg s1, Arg s2, Arg s3, Arg s4, Arg s5, Arg s6, Arg s7 )-> ShortFixedString
  {
    ShortFixedString result;
    result += s0;
    result += s1;
    result += s2;
    result += s3;
    result += s4;
    result += s5;
    result += s6;
    result += s7;
    return result;
  }

  auto ShortFixedString::Concat( Arg s0, Arg s1, Arg s2, Arg s3, Arg s4, Arg s5, Arg s6 )-> ShortFixedString
  {
    ShortFixedString result;
    result += s0;
    result += s1;
    result += s2;
    result += s3;
    result += s4;
    result += s5;
    result += s6;
    return result;
  }

  auto ShortFixedString::Concat( Arg s0, Arg s1, Arg s2, Arg s3, Arg s4, Arg s5 )-> ShortFixedString
  {
    ShortFixedString result;
    result += s0;
    result += s1;
    result += s2;
    result += s3;
    result += s4;
    result += s5;
    return result;
  }

  auto ShortFixedString::Concat( Arg s0, Arg s1, Arg s2, Arg s3, Arg s4 )-> ShortFixedString
  {
    ShortFixedString result;
    result += s0;
    result += s1;
    result += s2;
    result += s3;
    result += s4;
    return result;
  }

  auto ShortFixedString::Concat( Arg s0, Arg s1, Arg s2, Arg s3)-> ShortFixedString
  {
    ShortFixedString result;
    result += s0;
    result += s1;
    result += s2;
    result += s3;
    return result;
  }

  auto ShortFixedString::Concat( Arg s0, Arg s1, Arg s2)-> ShortFixedString
  {
    ShortFixedString result;
    result += s0;
    result += s1;
    result += s2;
    return result;
  }

  auto ShortFixedString::Concat( Arg s0, Arg s1)-> ShortFixedString
  {
    ShortFixedString result;
    result += s0;
    result += s1;
    return result;
  }

  //operator const char* ( ) const;

  bool ShortFixedString::empty() const                                      { return !mSize; }
  auto ShortFixedString::size() const -> int                                { return mSize; }
  auto ShortFixedString::capacity() const -> int                            { return N; }
  auto ShortFixedString::data() const -> const char*                        { return mBuf; }
  void ShortFixedString::assign( const StringView sv )                     { FixedStringAssign( GetData(), sv ); }
  auto ShortFixedString::front() dynmc -> dynmc char&                       { return mBuf[ 0 ]; }
  auto ShortFixedString::front() const -> const char&                       { return mBuf[ 0 ]; }
  auto ShortFixedString::back() dynmc -> dynmc char&                        { return mBuf[ mSize - 1 ]; }
  auto ShortFixedString::back() const -> const char&                        { return mBuf[ mSize - 1 ]; }
  auto ShortFixedString::begin() dynmc -> dynmc char*                       { return mBuf; }
  auto ShortFixedString::begin() const -> const char*                       { return mBuf; }
  auto ShortFixedString::end()  dynmc -> dynmc char*                        { return mBuf + mSize;}
  auto ShortFixedString::end() const -> const char*                         { return mBuf + mSize;}
  auto ShortFixedString::operator []( int i ) dynmc -> dynmc char&          { return mBuf[ i ]; }
  auto ShortFixedString::operator []( int i ) const -> const char&          { return mBuf[ i ]; }
  auto ShortFixedString::operator += ( char c ) -> ShortFixedString&        { FixedStringAppend( GetData(), StringView( &c, 1 ) ); return *this; }
  auto ShortFixedString::operator += ( StringView sv ) -> ShortFixedString& { FixedStringAppend( GetData(), sv ); return *this; }

  ShortFixedString::operator StringView() const               { return StringView( mBuf, mSize ); }

  // needed to convert the FixedString result from va()
  // into the const char* in TAC_ASSERT HandleAssert
  //ShortFixedString::operator const char*() const               { return mBuf; }

  auto ShortFixedString::GetData() -> Data
  {
    return Data
    {
      .mBuf      { mBuf },
      .mSize     { &mSize },
      .mCapacity { N },
    };
  }

} // namespace Tac



