#pragma once

#include <iostream> // temp?
#include <initializer_list>

struct TacString;
struct TacStringView;

bool TacIsSpace( char c );
bool TacIsAlpha( char c );

int TacStrLen( const char* str );
// Negative value if lhs appears before rhs in lexicographical order.
// Zero if lhs and rhs compare equal.
// Positive value if lhs appears after rhs in lexicographical order.
int TacStrCmp( const char* lhs, const char* rhs );
int TacMemCmp( const void* lhs, const void* rhs, int len );

// std uses void*
void TacMemCpy( void* dst, const void* src, int len );

struct TacStringView
{
  TacStringView( const char* str );
  TacStringView( const char* str, int len );
  TacStringView( const TacString& str );
  char operator[]( int i ) const;
  const char* data() const;
  int size() const;
  const char* begin() const;
  const char* end() const;
  const char* c_str() const;

  const char* mStr = nullptr;
  int mLen = 0;
};

struct TacString
{
  TacString();
  TacString( const TacStringView& rhs );
  TacString( const TacString& rhs );
  TacString( const char* begin, const char* end );
  TacString( const char* str, int len );
  TacString( int len, char c );
  TacString( const char* str );
  ~TacString();
  void clear();
  bool empty() const;
  char* c_str() const;
  int size() const;
  char* data() const;
  char operator[]( int i ) const;
  char& operator[]( int i );
  void operator = ( const TacString& str );
  void operator = ( const char* str );
  void operator += ( const char* str );
  void operator += ( const TacString& s );
  void operator += ( char c );
  void push_back( char c );
  void assign( const char* str, int strLen );
  void append( const char* str, int strLen );
  void append( const TacString& s );
  void prepend( const TacString& s );
  void reserve( int lenNotIncNull );
  void resize( int lenNotIncNull );
  char* begin() const;
  char* end() const;
  // returns npos if not found
  int find_last_of( const char* c ) const;
  int find( const TacString& substr ) const;
  TacString substr( int pos = 0, int len = npos ) const;

  static const int npos = -1; // mimicking the standard library
  char* mStr = nullptr;
  int mLen = 0; // number of bytes before the null-terminator
  int mAllocatedByteCount = 0; // includes the null-terminator
};

TacString TacToString( int i );
TacString TacToString( void* val );
TacString TacToString( double val );
TacString TacToString( float val );
TacString TacToString( uint32_t val );
TacStringView TacVa( const char* format, ... );

TacString TacJoin( const TacString&, std::initializer_list< TacString > );
TacString TacJoin( const TacString&, const TacString*, int );

template< typename T >
TacString TacJoin( const TacString& sep, const T& ts )
{
  TacString result;
  for( int i = 0; i < ts.size(); ++i )
  {
    if( i )
    {
      result += sep;
    }
    result += ts[ i ];
  }
  return result;
}

TacString operator + ( char c, const TacString& s );
TacString operator + ( const TacString& s, char c );
TacString operator + ( const TacString& s, const char* c );
TacString operator + ( const TacString& lhs, const TacString& rhs );
bool operator == ( const TacString& a, const TacString& b );
bool operator != ( const TacString& a, const TacString& b );
bool operator < ( const TacString& a, const TacString& b );

std::ostream& operator << ( std::ostream& os, const TacString& s ); // temp?
std::istream& operator >> ( std::istream& is, TacString& s ); // temp?
