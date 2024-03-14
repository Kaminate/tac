#include "tac_string.h" // self-inc

#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/tac_ints.h"
//#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/string/tac_string_format.h"
#include "tac-std-lib/string/tac_string_view.h"

import std; // <string>

  // -----------------------------------------------------------------------------------------------

namespace Tac
{

  static String ItoaU64( u64 val, int base = 10 )
  {
    if( !val )
      return "0";

    TAC_ASSERT( base >= 2 || base <= 16);

    String s;
    while( val )
    {
      const int i = val % base;
      const char c = "0123456789abcdef"[ i ];
      val /= base;
      s.push_back(c);
    }

    Reverse( s.begin(), s.end() );
    return s;
  }
  static String ItoaI64( i64 val, int base = 10 )
  {
    const String s = ItoaU64( ( u64 )val, base );
    return val < 0 ? '-' + s : s;
  }

}


bool        Tac::IsSpace( const char c )
{
  return StringView( " \t\n\v\f\r" ).find_first_of( c ) != StringView::npos;
}

bool        Tac::IsDigit( const char c )
{
  return c >= '0' && c <= '9';
}

bool        Tac::IsAlpha( const char c )
{
  return
    ( c >= 'A' && c <= 'Z' ) ||
    ( c >= 'a' && c <= 'z' );
}

int         Tac::MemCmp( const void* lhs, const void* rhs, int len )
{
  for( int i = 0; i < len; ++i )
  {
    int diff = ( ( const char* )lhs )[ i ] - ( ( const char* )rhs )[ i ];
    if( diff )
      return diff;
  }
  return 0;
}

void        Tac::MemCpy( void* dst, const void* src, int len )
{
  auto* dstChar = ( char* )dst;
  auto* srcChar = ( char* )src;
  while( len-- )
    *dstChar++ = *srcChar++;
}

void        Tac::MemSet( void* dst, unsigned char c, int n )
{
  for( int i = 0; i < n; ++i )
    ( ( char* )dst )[ i ] = c;
}

int         Tac::StrCmp( const char* lhs, const char* rhs )
{
  while( *lhs && ( *lhs == *rhs ) )
  {
    ++lhs;
    ++rhs;
  }
  return *lhs - *rhs;
}

int         Tac::StrLen( const char* str )
{
  // do not handle null string
  int result = 0;
  while( *str++ )
    ++result;
  return result;
}

void        Tac::StrCpy( char* dst, const char* src )
{
  for( ;; )
  {
    *dst = *src;
    if( *src == '\0' )
      break;
    dst++;
    src++;
  }
}

Tac::String Tac::Itoa( int val, int base )
{
  return ItoaI64( ( i64 )val, base );
}

float       Tac::Atof( const StringView& s )
{
  return (float)std::atof( s.data() );
}

int         Tac::Atoi( const StringView& s )
{
  int res = 0;
  int pow = 1;
  for( int i = 0; i < s.size(); ++i, pow *= 10 )
    res += ( s[ s.size() - i - 1 ] - '0' ) * pow;
  return res;
}

Tac::String Tac::ToString( unsigned int val )       { return ItoaU64( ( u64 )val ); }
Tac::String Tac::ToString( unsigned long long val ) { return ItoaU64( ( u64 )val ); }
Tac::String Tac::ToString( int val )                { return Itoa( val ); }
Tac::String Tac::ToString( char c )                 { return String( 1, c ); }
Tac::String Tac::ToString( const void* val )        { return "0x" + ItoaU64( ( u64 )val ); }
Tac::String Tac::ToString( double val )            
{
  const bool isNegative = val < 0;
  if( isNegative )
    val *= -1;

  if( val < 0.001 )
    return "0";

  const auto integralPart = ( u64 )val;
  const auto fractionalPart = ( u64 )( ( val - integralPart ) * 1000 );

  return String()
    + ( isNegative ? "-" : "" )
    + ItoaU64( integralPart )
    + "."
    + ItoaU64( fractionalPart );
}

Tac::String Tac::ToString( float val )             
{
  return ToString( (double)val);
}

// -------------------------------------------------------------------------------------------------

namespace Tac
{

  //String::String()                 { clear(); }
  String::String( StringView rhs ) { append( rhs.mStr, rhs.mLen ); }
  String::String( const String& rhs )
  {
    *this = rhs;
    //mLen = rhs.mLen;
    //mStr = new char[ mAllocatedByteCount = mLen + 1 ];
    //MemCpy( mStr, rhs.mStr, mLen );
    //mStr[ mLen ] = '\0';
  }
  String::String( int len, char c )
  {
    reserve( len );
    for( int i = 0; i < len; ++i )
      mStr[ i ] = c;
    mStr[ mLen = len ] = '\0';
    //mLen = len;
  }
  String::String( const char* begin, const char* end ) { append( begin, ( int )( end - begin ) ); }
  String::String( const char* str, int len )           { append( str, len ); }
  String::String( const char* str )                    { assign( str ); }
  String::~String()
  {
    if( mStr != mSSOBuffer )
      TAC_DELETE[] mStr;
    mStr = mSSOBuffer;
    mLen = 0;
  }

  //String::operator const char* () const          { return mStr; }
  void         String::clear()                   { assign( "" ); }
  bool         String::empty() const             { return !mLen; }
  const char*  String::c_str() const             { return mStr; }
  int          String::size() const              { return mLen; }
  const char*  String::data() const              { return mStr; }
  char         String::operator[]( int i ) const { return mStr[ i ]; }
  char&        String::operator[]( int i )       { return mStr[ i ]; }
  void         String::reserve( int newLen )
  {
    const int newCapacity = newLen + 1;
    if( newCapacity <= mCapacity )
      return;
    auto* newStr = TAC_NEW char[ newCapacity ];
    MemCpy( newStr, mStr, mLen );
    newStr[ mLen ] = '\0';
    if( mStr != mSSOBuffer )
      TAC_DELETE[] mStr;
    mStr = newStr;
    mCapacity = newCapacity;
  }
  void         String::resize( int lenNotIncNull )
  {
    reserve( lenNotIncNull );
    mStr[ mLen = lenNotIncNull ] = '\0';
  }

  void         String::replace( StringView a, StringView b)
  {
    String result;
    StringView remainder = *this;
    for( ;; )
    {
      const int i = remainder.find( a );
      if( i == npos )
        break;

      result += remainder.substr( 0, i );
      result += b;

      remainder.remove_prefix( i + a.size() );
    }

    result += remainder;
    *this = result;
  }

  void         String::pop_back() { mStr[ --mLen ] = '\0'; }

  int          String::find_last_of( const char* c ) const
  {
    int cLen = StrLen( c );
    int iFound = npos;
    for( int i = 0; i < mLen; ++i )
    {
      char myStrChar = mStr[ i ];
      for( int j = 0; j < cLen; ++j )
      {
        char queryChar = c[ j ];
        if( myStrChar == queryChar )
        {
          iFound = i;
        }
      }
    }
    return iFound;
  }
  int          String::find( const String& substr ) const
  {
    if( substr.mLen > mLen )
      return npos;
    for( int i = 0; i < mLen - substr.mLen; ++i )
      if( MemCmp( mStr + i, substr.mStr, substr.mLen ) == 0 )
        return i;
    return npos;
  }
  int          String::find( char c ) const { return StringView( this->data(), this->size() ).find_first_of( c ); }
  bool         String::contains( const StringView& s ) const { return npos != find( s ); }
  bool         String::contains( char c ) const                   { return npos != find( c ); }
  String       String::substr( int pos, int len ) const
  {
    int remainingLen = mLen - pos;
    int resultLen = len == npos ? remainingLen : Min( remainingLen, len );
    String result( mStr + pos, resultLen );
    return result;
  }
  void         String::operator = ( const char* str )        { assign( StringView( str ) ); }
  void         String::operator = ( const String& str )      { assign( str.c_str(), str.size() ); }
  void         String::operator = ( const StringView& str )  { assign( str.data(), str.size() ); }
  //void   String::operator += ( const char* str )       { append( str, StrLen( str ) ); }
  //void   String::operator += ( const String& s )       { append( s.mStr, s.mLen ); }
  void         String::operator += ( char c )                { append( &c, 1 ); }
  void         String::operator += ( const StringView& sv ) { append( sv.data(), sv.size() ); }

  void         String::erase( int pos, int len )
  {
    const int end_pos = len == npos ? mLen : pos + len;
    String copy;
    for( int i = 0; i < mLen; ++i )
      if( i < pos || i >= end_pos )
        copy.push_back( mStr[ i ] );
    *this = copy;
  }

  void         String::push_back( char c )               { append( &c, 1 ); }
  bool         String::starts_with( StringView s ) const { return mLen >= s.mLen && MemCmp( mStr, s.mStr, s.mLen ) == 0; }
  bool         String::starts_with( char c ) const       { return mLen && mStr[ 0 ] == c; }
  bool         String::ends_with( StringView s ) const
  {
    return mLen >= s.mLen && MemCmp( mStr + mLen - s.mLen, s.mStr, s.mLen ) == 0;
  }

  void         String::assign( const StringView& s)
  {
    assign( s.data(), s.size() );
  }

  void         String::assign( const char* str, int len )
  {
    reserve( len );
    MemCpy( mStr, str, len );
    mStr[ mLen = len ] = '\0';
  }

  void         String::append( const char* str, int len )
  {
    int newLen = mLen + len;
    reserve( newLen );
    MemCpy( mStr + mLen, str, len );
    mStr[ mLen = newLen ] = '\0';
  }
  void         String::append( const char* str)
  {
    append( str, StrLen( str ) );
  }
  void         String::append( const String& s )      { *this += s;               }
  void         String::prepend( const String& s )     { *this = s + *this;        }
  char*        String::begin() const                  { return mStr;              }
  char*        String::end() const                    { return mStr + mLen;       }
  char&        String::back()                         { return mStr[ mLen - 1 ];  }
  char&        String::front()                        { return *mStr;             }
  int          String::compare( const char* s ) const { return StrCmp( mStr, s ); }

  // This constexpr implicit conversion function, which calls constexpr StringView(),
  // allows for String() == StringView()
  //constexpr String::operator StringView() const noexcept { return StringView( mStr, mLen ); }

} // namespace Tac

bool          Tac::operator == ( const StringView& a, const StringView& b )
{
  return a.size() == b.size() && !MemCmp( a.data(), b.data(), a.size() );
}

//bool          Tac::operator == ( const StringView& a, const String& b ) { return a == b; }
//bool          Tac::operator == ( const String& a, const StringView& b ) { return ( StringView )a == b; }
//bool          Tac::operator == ( const String& a, const char* b )       { return ( StringView )a == StringView( b ); }
//bool          Tac::operator == ( const StringView& a, const char* b )   { return a == StringView( b ); }
//bool          Tac::operator == ( const char* a, const StringView& b )   { return StringView( a ) == b; }
Tac::String   Tac::operator + ( char c, const String& s )
{
  String result;
  result += c;
  result += s;
  return result;
}
Tac::String   Tac::operator + ( const String& s, char c )
{
  String result = s;
  result += c;
  return result;
}
Tac::String   Tac::operator + ( const String& s, const char* c )
{
  String result = s;
  result += c;
  return result;
}
Tac::String   Tac::operator + ( const String& lhs, const String& rhs )
{
  String result;
  result += lhs;
  result += rhs;
  return result;
}
Tac::String   Tac::operator + ( const String& lhs, const StringView& rhs )
{
  String result = lhs;
  result += rhs;
  return result;
}
Tac::String   Tac::operator + ( const StringView& a, const char* b )   { return String( a ) + String( b ); }
Tac::String   Tac::operator + ( const StringView& a, const String& b ) { return String( a ) + b; }
Tac::String   Tac::operator + ( const char* a, const String& b )       { return String( a ) + b; }
Tac::String   Tac::operator + ( const char* a, const StringView& b )   { return String( a ) + String( b ); }
bool          Tac::operator == ( const String& a, const String& b )
{
  return a.mLen == b.mLen && 0 == StrCmp( a.mStr, b.mStr );
}
bool          Tac::operator != ( const String& a, const String& b ) { return !( a == b ); }
bool          Tac::operator < ( const String& a, const String& b )  { return StrCmp( a.c_str(), b.c_str() ) < 0; }
bool          Tac::operator > ( const String& a, const String& b )  { return StrCmp( a.c_str(), b.c_str() ) > 0; }




