
#include "src/common/tacString.h"
#include "src/common/math/tacMath.h"
#include "src/common/tacAlgorithm.h"


#include <string>
#include <algorithm>
#include <sstream>
#include <cstdarg>

namespace Tac
{

  bool IsSpace( char c )
  {
    bool result =
      c == ' ' || // space
      c == '\t' || // horizontal tab
      c == '\n' || // newline
      c == '\v' || // vertical tab
      c == '\f' || // feed
      c == '\r'; // carriage return
    return result;
  }
  bool IsAlpha( char c )
  {
    return
      ( c >= 'A' && c <= 'Z' ) ||
      ( c >= 'a' && c <= 'z' );
  }

  int MemCmp( const void* lhs, const void* rhs, int len )
  {
    for( int i = 0; i < len; ++i )
    {
      int diff = ( ( const char* )lhs )[ i ] - ( ( const char* )rhs )[ i ];
      if( diff )
        return diff;
    }
    return 0;
  }

  int StrCmp( const char* lhs, const char* rhs )
  {
    while( *lhs && ( *lhs == *rhs ) )
    {
      ++lhs;
      ++rhs;
    }
    return *lhs - *rhs;
  }

  int StrLen( const char* str )
  {
    int result = 0;
    while( *str++ )
      ++result;
    return result;
  }

  void MemCpy( void* dst, const void* src, int len )
  {
    auto* dstChar = ( char* )dst;
    auto* srcChar = ( char* )src;
    while( len-- )
      *dstChar++ = *srcChar++;
  }

  void StrCpy( char* dst, const char* src )
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


  template <typename T> String ToStringUnsignedInt( T i )
  {
    String s;
    for( ;; )
    {
      char curdigit = i % 10 + '0';
      s = curdigit + s;
      i /= 10;
      if( !i )
        break;
    }
    return s;
  }

  template <typename T> String ToStringSignedInt( T i )
  {
    return ( i < 0 ? "-" : "" ) + ToStringUnsignedInt( ( i < 0 ? -1 : 1 ) * i );
  }

  String     ToString( unsigned int i )
  {
    return ToStringUnsignedInt( i );
  }

  String     ToString( unsigned long long i )
  {
    return ToStringUnsignedInt( i );
  }

  String ToString( int i )
  {
    return ToStringSignedInt( i );
  }

  String ToString( void* val )
  {
    std::stringstream ss;
    ss << val;
    return ss.str().c_str();
  }
  String ToString( double val )
  {
    return std::to_string( val ).data();
  }
  String ToString( float val )
  {
    return std::to_string( val ).data();
  }


  StringView::StringView( const char* str )
  {
    mStr = str;
    mLen = str ? StrLen( str ) : 0;
  }
  StringView::StringView( const char* str, int len )
  {
    mStr = str;
    mLen = len;
  }
  StringView::StringView( const char* strBegin, const char* strEnd )
  {
    mStr = strBegin;
    mLen = ( int )( strEnd - strBegin );
  }
  StringView::StringView( const String& str )
  {
    mStr = str.mStr;
    mLen = str.mLen;
  }
  char StringView::operator[]( int i ) const
  {
    return mStr[ i ];
  }
  const char* StringView::data() const
  {
    return mStr;
  }
  int StringView::size() const
  {
    return mLen;
  }
  const char* StringView::begin() const
  {
    return mStr;
  }
  const char* StringView::end() const
  {
    return mStr + mLen;
  }
  const char* StringView::c_str() const
  {
    return data();
  }
  bool StringView::empty() const
  {
    return mLen == 0;
  }
  int StringView::find_last_of( StringView s ) const
  {
    for( int i = 0; i < mLen; ++i )
      if( Contains( s, mStr[ mLen - i - 1 ] ) )
        return i;
    return npos;
  }

  int StringView::find_first_of( StringView s ) const
  {
    for( int i = 0; i < mLen; ++i )
      if( Contains( s, mStr[ i ] ) )
        return i;
    return npos;
  }

  StringView StringView::substr( const int pos,
                                 const int len ) const
  {
    return StringView( mStr + pos,
                       len == npos ? mLen - pos : len );
  }
    char StringView::front()
    {
      return mStr[ 0 ];
    }
    char StringView::back()
    {
      return mStr[ mLen - 1 ];
    }


  void StringView::remove_prefix( const int n )
  {
    mStr += n;
    mLen -= n;
  }

  void StringView::remove_suffix( const int n )
  {
    mLen -= n;
  }

  bool StringView::starts_with( StringView s )
  {
    return mLen >= s.mLen && MemCmp( mStr, s.mStr, s.mLen ) == 0;
  }

  bool StringView::starts_with( char c )
  {
    return mLen && mStr[ 0 ] == c;
  }

  bool StringView::ends_with( StringView s )
  {
    return mLen >= s.mLen && MemCmp( mStr + mLen - s.mLen, s.mStr, s.mLen ) == 0;
  }

  String::String()
  {
    clear();
  }

  String::String( StringView rhs )
  {
    append( rhs.mStr, rhs.mLen );
  }
  String::String( const String& rhs )
  {
    mLen = rhs.mLen;
    mStr = new char[ mAllocatedByteCount = mLen + 1 ];
    MemCpy( mStr, rhs.mStr, mLen );
    mStr[ mLen ] = '\0';
  }
  String::String( int len, char c )
  {
    reserve( len );
    for( int i = 0; i < len; ++i )
      mStr[ i ] = c;
    mLen = len;
  }
  String::String( const char* begin, const char* end )
  {
    auto len = ( int )( end - begin );
    append( begin, len );
  }
  String::String( const char* str, int len )
  {
    append( str, len );
  }
  String::String( const char* str )
  {
    *this = str;
  }
  StringView Va( const char* format, ... )
  {
    // todo: move this to frame allocator
    const int bufSize = 1024 * 1024;
    static char buf[ bufSize ];
    static int i;

    const int strSize = 256;
    int iBegin = i + strSize > bufSize ? 0 : i;
    int iEnd = iBegin + strSize;
    i = iEnd;

    std::va_list args;
    va_start( args, format );
    std::vsnprintf( buf + iBegin, strSize, format, args );
    va_end( args );
    return buf + iBegin;
  }

  int        Atoi( StringView s )
  {
    int res = 0;
    int pow = 1;
    for( int i = 0; i < s.size(); ++i, pow *= 10 )
      res += ( s[ s.size() - i - 1 ] - '0' ) * pow;
    return res;
  }

  String::~String()
  {
    delete[] mStr;
    mStr = nullptr;
    mLen = 0;
  }
  void String::clear()
  {
    *this = "";
  }

  bool String::empty() const
  {
    return !mLen;
  }

  char* String::c_str() const
  {
    return mStr;
  }

  int String::size() const
  {
    return mLen;
  }

  char* String::data() const
  {
    return mStr;
  }

  char String::operator[]( int i ) const
  {
    return mStr[ i ];
  }

  char& String::operator[]( int i )
  {
    return mStr[ i ];
  }

  void String::reserve( int newLen )
  {
    if( mStr && newLen <= mLen )
      return;
    int newByteCount = newLen + 1;
    auto* newStr = new char[ newByteCount ];
    MemCpy( newStr, mStr, mLen );
    newStr[ mLen ] = '\0';
    delete[] mStr;
    mStr = newStr;
    mAllocatedByteCount = newByteCount;
  }

  void String::resize( int lenNotIncNull )
  {
    reserve( lenNotIncNull );
    mStr[ mLen = lenNotIncNull ] = '\0';
  }

  int String::find_last_of( const char* c ) const
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
  int String::find( const String& substr ) const
  {
    if( substr.mLen > mLen )
      return npos;
    for( int i = 0; i < mLen - substr.mLen; ++i )
    {
      if( MemCmp( mStr + i, substr.mStr, substr.mLen ) == 0 )
        return i;
    }
    return npos;
  }

  String String::substr( int pos, int len ) const
  {
    int remainingLen = mLen - pos;
    int resultLen = len == npos ? remainingLen : Min( remainingLen, len );
    String result( mStr + pos, resultLen );
    return result;
  }


  void String::operator = ( const char* str )
  {
    int newStrLen = StrLen( str );
    assign( str, newStrLen );
  }
  void String::operator = ( const String& str )
  {
    assign( str.c_str(), str.size() );
  }
  void String::operator = ( StringView str )
  {
    assign( str.data(), str.size() );
  }
  void String::operator += ( const char* str )
  {
    int len = StrLen( str );
    append( str, len );
  }
  void String::operator += ( const String& s )
  {
    append( s.mStr, s.mLen );
  }
  void String::operator += ( char c )
  {
    append( &c, 1 );
  }

  String operator + ( char c, const String& s )
  {
    String result;
    result += c;
    result += s;
    return result;
  }
  String operator + ( const String& s, char c )
  {
    String result = s;
    result += c;
    return result;
  }
  String operator + ( const String& s, const char* c )
  {
    String result = s;
    result += c;
    return result;
  }
  String operator + ( const String& lhs, const String& rhs )
  {
    String result;
    result += lhs;
    result += rhs;
    return result;
  }

  bool operator == ( const String& a, const String& b )
  {
    if( a.mLen != b.mLen )
      return false;
    bool result = !StrCmp( a.mStr, b.mStr );
    return result;
  }
  bool operator != ( const String& a, const String& b )
  {
    return !( a == b );
  }
  bool operator < ( const String& a, const String& b )
  {
    return StrCmp( a.c_str(), b.c_str() ) < 0;
  }

  void String::push_back( char c )
  {
    append( &c, 1 );
  }
  void String::assign( const char* str, int len )
  {
    reserve( len );
    MemCpy( mStr, str, len );
    mStr[ mLen = len ] = '\0';
  }
  void String::append( const char* str, int len )
  {
    int newLen = mLen + len;
    reserve( newLen );
    MemCpy( mStr + mLen, str, len );
    mStr[ mLen = newLen ] = '\0';
  }
  void String::append( const String& s )
  {
    *this += s;
  }
  void String::prepend( const String& s )
  {
    *this = s + *this;
  }

  char* String::begin() const
  {
    return mStr;
  }
  char* String::end() const
  {
    return mStr + mLen;
  }

  std::ostream& operator << ( std::ostream& os, StringView s )
  {
    return os << s.c_str();
  }
  std::istream& operator >> ( std::istream& is, String& s )
  {
    char c;
    while( is >> c )
    {
      if( IsSpace( c ) )
      {
        if( !s.empty() )
          break;
      }
      else
      {
        s += c;
      }
    }
    return is;
  }

  bool operator == ( const StringView& a, const StringView& b )
  {
    return a.size() == b.size() && !MemCmp( a.data(), b.data(), a.size() );
  }
  bool operator == ( const StringView& a, const String& b )
  {
    return a == StringView( b );
  }
  bool operator == ( const String& a, const StringView& b )
  {
    return StringView( a ) == b;
  }
  bool operator == ( const String& a, const char* b )
  {
    return StringView( a ) == StringView( b );
  }
  bool operator == ( const StringView& a, const char* b )
  {
    return a == StringView( b );
  }

  const int kMaxStringDictionaryEntries = 1024;
  String gStringLookup[ kMaxStringDictionaryEntries ];

  StringView DebugLookupString( StringID stringID )
  {
    return gStringLookup[ stringID.mHash ];
  }


  int StringID::Hash( StringView stringView )
  {
    uint32_t hash = 0;
    for( char c : stringView )
      hash = 37 * hash + c;
    hash %= kMaxStringDictionaryEntries;
    return hash;
  }

  StringID::StringID( StringView stringView )
  {
    mHash = Hash( stringView );
    TAC_ASSERT( mHash >= 0 && mHash < kMaxStringDictionaryEntries );
    if( IsDebugMode() && mHash && gStringLookup[ mHash ].empty() )
      gStringLookup[ mHash ] = stringView;
  }

  bool operator < ( StringID a, StringID b )
  {
    return a.mHash < b.mHash;
  }

  bool operator == ( StringID a, StringID b )
  {
    return a.mHash == b.mHash;
  }


  //String Join( const String& separator, std::initializer_list< String > strings )
  //{
  //  const auto stringCount = strings.size();
  //  return Join( separator, strings.begin(), ( int )strings.size() );
  //}

  //String Join( const String& separator, const String* strings, int stringCount )
  //{
  //  String result;
  //  for( int i = 0; i < stringCount; ++i )
  //  {
  //    if( i )
  //      result += separator;
  //    result += strings[ i ];
  //  }
  //  return result;
  //}
}
