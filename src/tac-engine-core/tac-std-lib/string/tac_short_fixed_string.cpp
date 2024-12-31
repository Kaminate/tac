#include "tac_short_fixed_string.h" // self-inc

#include "tac-std-lib/string/tac_string_view.h"

namespace Tac
{

  static void FixedStringAppend( const ShortFixedString::Data& data, const StringView& sv )
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

  static void FixedStringAssign( const ShortFixedString::Data& data, const StringView& sv )
  {
    *data.mSize = 0;
    FixedStringAppend( data, sv );
  }


  ShortFixedString::ShortFixedString( const char* s )        { assign( s ); }
  ShortFixedString::ShortFixedString( const char* s, int n ) { assign( StringView( s, n ) ); }
  ShortFixedString::ShortFixedString( StringView sv ) { assign( sv ); }

  ShortFixedString ShortFixedString::Concat( Arg s0, Arg s1, Arg s2, Arg s3, Arg s4, Arg s5, Arg s6, Arg s7 )
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

  ShortFixedString ShortFixedString::Concat( Arg s0, Arg s1, Arg s2, Arg s3, Arg s4, Arg s5, Arg s6 )
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

  ShortFixedString ShortFixedString::Concat( Arg s0, Arg s1, Arg s2, Arg s3, Arg s4, Arg s5 )
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

  ShortFixedString ShortFixedString::Concat( Arg s0, Arg s1, Arg s2, Arg s3, Arg s4 )
  {
    ShortFixedString result;
    result += s0;
    result += s1;
    result += s2;
    result += s3;
    result += s4;
    return result;
  }

  ShortFixedString ShortFixedString::Concat( Arg s0, Arg s1, Arg s2, Arg s3)
  {
    ShortFixedString result;
    result += s0;
    result += s1;
    result += s2;
    result += s3;
    return result;
  }

  ShortFixedString ShortFixedString::Concat( Arg s0, Arg s1, Arg s2)
  {
    ShortFixedString result;
    result += s0;
    result += s1;
    result += s2;
    return result;
  }

  ShortFixedString ShortFixedString::Concat( Arg s0, Arg s1)
  {
    ShortFixedString result;
    result += s0;
    result += s1;
    return result;
  }

  //operator const char* ( ) const;

  bool        ShortFixedString::empty() const                  { return !mSize; }
  int         ShortFixedString::size() const                   { return mSize; }
  int         ShortFixedString::capacity() const               { return N; }
  const char* ShortFixedString::data() const                   { return mBuf; }
  void        ShortFixedString::assign( const StringView& sv ) { FixedStringAssign( GetData(), sv ); }
  dynmc char& ShortFixedString::front() dynmc                  { return mBuf[ 0 ]; }
  const char& ShortFixedString::front() const                  { return mBuf[ 0 ]; }
  dynmc char& ShortFixedString::back() dynmc                   { return mBuf[ mSize - 1 ]; }
  const char& ShortFixedString::back() const                   { return mBuf[ mSize - 1 ]; }
  dynmc char* ShortFixedString::begin() dynmc                  { return mBuf; }
  const char* ShortFixedString::begin() const                  { return mBuf; }
  dynmc char* ShortFixedString::end()  dynmc                   { return mBuf + mSize;}
  const char* ShortFixedString::end() const                    { return mBuf + mSize;}
  dynmc char& ShortFixedString::operator []( int i ) dynmc     { return mBuf[ i ]; }
  const char& ShortFixedString::operator []( int i ) const     { return mBuf[ i ]; }
  ShortFixedString& ShortFixedString::operator += ( char c )                { FixedStringAppend( GetData(), StringView( &c, 1 ) ); return *this; }
  ShortFixedString& ShortFixedString::operator += ( const StringView& sv )  { FixedStringAppend( GetData(), sv ); return *this; }

  ShortFixedString::operator StringView() const               { return StringView( mBuf, mSize ); }

  // needed to convert the FixedString result from va()
  // into the const char* in TAC_ASSERT HandleAssert
  //ShortFixedString::operator const char*() const               { return mBuf; }

  ShortFixedString::Data ShortFixedString::GetData()
  {
    return Data
    {
      .mBuf      { mBuf },
      .mSize     { &mSize },
      .mCapacity { N },
    };
  }

} // namespace Tac



