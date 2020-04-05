#pragma once

#include <iostream> // temp?
#include <initializer_list>

namespace Tac
{

  struct String;
  struct StringView;

  bool IsSpace( char c );
  bool IsAlpha( char c );

  int StrLen( const char* str );
  // Negative value if lhs appears before rhs in lexicographical order.
  // Zero if lhs and rhs compare equal.
  // Positive value if lhs appears after rhs in lexicographical order.
  int StrCmp( const char* lhs, const char* rhs );
  int MemCmp( const void* lhs, const void* rhs, int len );

  // std uses void*
  void MemCpy( void* dst, const void* src, int len );

  struct StringView
  {
    StringView( const char* str );
    StringView( const char* str, int len );
    StringView( const String& str );
    char operator[]( int i ) const;
    const char* data() const;
    int size() const;
    const char* begin() const;
    const char* end() const;
    const char* c_str() const;

    const char* mStr = nullptr;
    int mLen = 0;
  };

  struct String
  {
    String();
    String( const StringView& rhs );
    String( const String& rhs );
    String( const char* begin, const char* end );
    String( const char* str, int len );
    String( int len, char c );
    String( const char* str );
    ~String();
    void clear();
    bool empty() const;
    char* c_str() const;
    int size() const;
    char* data() const;
    char operator[]( int i ) const;
    char& operator[]( int i );
    void operator = ( const String& str );
    void operator = ( const char* str );
    void operator += ( const char* str );
    void operator += ( const String& s );
    void operator += ( char c );
    void push_back( char c );
    void assign( const char* str, int strLen );
    void append( const char* str, int strLen );
    void append( const String& s );
    void prepend( const String& s );
    void reserve( int lenNotIncNull );
    void resize( int lenNotIncNull );
    char* begin() const;
    char* end() const;
    // returns npos if not found
    int find_last_of( const char* c ) const;
    int find( const String& substr ) const;
    String substr( int pos = 0, int len = npos ) const;

    static const int npos = -1; // mimicking the standard library
    char* mStr = nullptr;
    int mLen = 0; // number of bytes before the null-terminator
    int mAllocatedByteCount = 0; // includes the null-terminator
  };

  String ToString( int i );
  String ToString( void* val );
  String ToString( double val );
  String ToString( float val );
  String ToString( uint32_t val );
  StringView Va( const char* format, ... );

  String Join( const String&, std::initializer_list< String > );
  String Join( const String&, const String*, int );

  template< typename T >
  String Join( const String& sep, const T& ts )
  {
    String result;
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

  String operator + ( char c, const String& s );
  String operator + ( const String& s, char c );
  String operator + ( const String& s, const char* c );
  String operator + ( const String& lhs, const String& rhs );
  bool operator == ( const String& a, const String& b );
  bool operator != ( const String& a, const String& b );
  bool operator < ( const String& a, const String& b );

  std::ostream& operator << ( std::ostream& os, const String& s ); // temp?
  std::istream& operator >> ( std::istream& is, String& s ); // temp?
}
