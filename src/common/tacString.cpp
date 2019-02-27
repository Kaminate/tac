
#include "common/tacString.h"
#include "common/math/tacMath.h"
#include "common/containers/tacVector.h"


#include <string>
#include <algorithm>
#include <sstream>

bool TacIsSpace( char c )
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

bool TacIsAlpha( char c )
{
  return
    ( c >= 'A' && c <= 'Z' ) ||
    ( c >= 'a' && c <= 'z' );
}

int TacStrCmp( const char* lhs, const char* rhs )
{
  while( *lhs && ( *lhs == *rhs ) )
  {
    ++lhs;
    ++rhs;
  }
  return *lhs - *rhs;
}
int TacStrLen( const char* str )
{
  int result = 0;
  while( *str++ )
    ++result;
  return result;
}
void TacMemCpy( void* dst, const void* src, int len )
{
  auto* dstChar = ( char* )dst;
  auto* srcChar = ( char* )src;
  while( len-- )
    *dstChar++ = *srcChar++;
}

TacString TacToString( int i )
{
  bool isNegative = i < 0;
  TacString s;
  for( ;; )
  {
    char curdigit = i % 10 + '0';
    s = curdigit + s;
    i /= 10;
    if( !i )
      break;
  }
  if( isNegative )
    s = "-" + s;
  return s;
}

TacString TacToString( uint32_t val )
{
  return TacToString( ( int )val );
}

TacString TacToString( void* val )
{
  std::stringstream ss;
  ss << val;
  return ss.str().c_str();
}

TacString TacToString( double val )
{
  return std::to_string( val ).data();
}
TacString TacToString( float val )
{
  return std::to_string( val ).data();
}

TacString::TacString()
{
  clear();
}

TacString::TacString( const TacString& rhs )
{
  mLen = rhs.mLen;
  mStr = new char[ mAllocatedByteCount = mLen + 1 ];
  TacMemCpy( mStr, rhs.mStr, mLen );
  mStr[ mLen ] = '\0';
}
TacString::TacString( int len, char c )
{
  reserve( len );
  for( int i = 0; i < len; ++i )
    mStr[ i ] = c;
  mLen = len;
}
TacString::TacString( const char* begin, const char* end )
{
    auto len = ( int )( end - begin );
  append( begin, len );
}
TacString::TacString( const char* str, int len )
{
  append( str, len );
}
TacString::TacString( const char* str )
{
  *this = str;
}
TacString::~TacString()
{
  delete[] mStr;
  mStr = nullptr;
  mLen = 0;
}
void TacString::clear()
{
  *this = "";
}

bool TacString::empty() const
{
  return !mLen;
}

char* TacString::c_str() const
{
  return mStr;
}

int TacString::size() const
{
  return mLen;
}

char* TacString::data() const
{
  return mStr;
}

char TacString::operator[]( int i ) const
{
  return mStr[ i ];
}

char& TacString::operator[]( int i )
{
  return mStr[ i ];
}

void TacString::reserve( int newLen )
{
  if( mStr && newLen <= mLen )
    return;
  int newByteCount = newLen + 1;
    auto* newStr = new char[ newByteCount ];
  TacMemCpy( newStr, mStr, mLen );
  newStr[ mLen ] = '\0';
  delete[] mStr;
  mStr = newStr;
  mAllocatedByteCount = newByteCount;
}

void TacString::resize( int lenNotIncNull )
{
  reserve( lenNotIncNull );
  mStr[ mLen = lenNotIncNull ] = '\0';
}

int TacString::find_last_of( const char* c ) const
{
  int cLen = TacStrLen( c );
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

TacString TacString::substr( int pos, int len ) const
{
  int remainingLen = mLen - pos;
  int resultLen = len == npos ? remainingLen : TacMin( remainingLen, len );
  TacString result( mStr + pos, resultLen );
  return result;
}


void TacString::operator = ( const char* str )
{
  int newStrLen = TacStrLen( str );
  assign( str, newStrLen );
}
void TacString::operator = ( const TacString& str )
{
  assign( str.c_str(), str.size() );
}
void TacString::operator += ( const char* str )
{
  int len = TacStrLen( str );
  append( str, len );
}
void TacString::operator += ( const TacString& s )
{
  append( s.mStr, s.mLen );
}
void TacString::operator += ( char c )
{
  append( &c, 1 );
}

TacString operator + ( char c, const TacString& s )
{
  TacString result;
  result += c;
  result += s;
  return result;
}
TacString operator + ( const TacString& s, char c )
{
  TacString result = s;
  result += c;
  return result;
}
TacString operator + ( const TacString& s, const char* c )
{
  TacString result = s;
  result += c;
  return result;
}
TacString operator + ( const TacString& lhs, const TacString& rhs )
{
  TacString result;
  result += lhs;
  result += rhs;
  return result;
}
bool operator == ( const TacString& a, const TacString& b )
{
  if( a.mLen != b.mLen )
    return false;
  bool result = !TacStrCmp( a.mStr, b.mStr );
  return result;
}

bool operator != ( const TacString& a, const TacString& b )
{
  return !( a == b );
}

bool operator < ( const TacString& a, const TacString& b )
{
  return TacStrCmp( a.c_str(), b.c_str() ) < 0;
}

void TacString::push_back( char c )
{
  append( &c, 1 );
}
void TacString::assign( const char* str, int len )
{
  reserve( len );
  TacMemCpy( mStr, str, len );
  mStr[ mLen = len ] = '\0';
}
void TacString::append( const char* str, int len )
{
  int newLen = mLen + len;
  reserve( newLen );
  TacMemCpy( mStr + mLen, str, len );
  mStr[ mLen = newLen ] = '\0';
}
void TacString::append( const TacString& s )
{
  *this += s;
}
void TacString::prepend( const TacString& s )
{
  *this = s + *this;
}

char* TacString::begin() const
{
  return mStr;
}
char* TacString::end() const
{
  return mStr + mLen;
}

std::ostream& operator << ( std::ostream& os, const TacString& s )
{
  return os << s.c_str();
}


std::istream& operator >> ( std::istream& is, TacString& s )
{
  char c;
  while( is >> c )
  {
    if( TacIsSpace( c ) )
    {
      if(!s.empty())
        break;
    }
    else
    {
      s += c;
    }
  }
  return is;
}


// TODO: make separator a const ref
TacString TacJoin( TacString separator, std::initializer_list< TacString > strings )
{
  TacVector< TacString > v( strings.begin(), strings.end() );
  return TacJoin( separator, v.data(), v.size() );
}

TacString TacJoin( TacString separator, const TacString* strings, int stringCount )
{
  TacString result;
  for( int i = 0; i < stringCount; ++i )
  {
    if( i )
      result += separator;
    result += strings[ i ];
  }
  return result;
}
