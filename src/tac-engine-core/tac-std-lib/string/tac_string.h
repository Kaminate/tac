#pragma once

#include "tac-std-lib/string/tac_string_view.h"

namespace Tac { struct String; }

namespace Tac
{
  bool IsSpace( char );
  bool IsAlpha( char );
  bool IsDigit( char );
  auto StrCmp( const char*, const char* ) -> int; // return <0, =0, or >0
  void StrCpy( char*, const char* );
  auto StrLen( const char* ) -> int;
  auto MemCmp( const void*, const void*, int ) -> int; // Returns 0 if they match
  void MemCpy( void*, const void*, int );
  void MemSet( void*, unsigned char, int );
  auto ToString( int ) -> String;
  auto ToString( char ) -> String;
  auto ToString( unsigned int ) -> String;
  auto ToString( unsigned long long ) -> String;
  auto ToString( const void* ) -> String;
  auto ToString( double ) -> String;
  auto ToString( float ) -> String;
  auto Atoi( StringView ) -> int;
  auto Atof( StringView ) -> float;
  auto Itoa( int, int base = 10 ) -> String;

  // -----------------------------------------------------------------------------------------------

  // A String represents a contiguous array of bytes representing readable text.
  // The encoding is assumed to be ascii, although there is nothing stopping you from stuffing
  // UTF-8 data in there, just note that internally it will be treated as ascii.
  struct String
  {
    String() = default;
    String( StringView );
    String( const String& );
    String( const char* begin, const char* end );
    String( const char*, int );
    String( int len, char c );
    String( const char* );
    ~String();
    auto operator[]( int ) const -> char;
    auto operator[]( int ) -> char&;
    void operator = ( const String& );
    void operator = ( StringView );
    void operator = ( const char* );
    void operator += ( StringView );
    void operator += ( char );

    void clear();
    bool empty() const;
    auto c_str() const -> const char*;
    auto data() const -> const char*;
    auto size() const -> int;
    void erase( int pos, int len = npos );
    bool starts_with( StringView ) const;
    bool starts_with( char ) const;
    bool ends_with( StringView ) const;
    bool ends_with( char ) const;
    void assign( const char*, int );
    void assign( StringView );
    void append( const char*, int );
    void append( const char* );
    void append( const String& );
    void prepend( const String& );
    void reserve( int lenNotIncNull );
    void resize( int lenNotIncNull );
    void replace( StringView, StringView );
    void push_back( char );
    void pop_back();
    auto begin() const -> char*;
    auto end() const -> char*;
    auto back() -> char&;
    auto back() const -> char;
    auto front() -> char&;
    auto front() const -> char;
    auto compare( const char* ) const -> int;
    auto find_last_of( const char* c ) const -> int; // returns npos if not found
    auto find( const String& substr ) const -> int;
    auto find( char ) const -> int;
    auto contains( StringView ) const -> bool;
    auto contains( char c ) const -> bool;
    auto substr( int pos = 0, int len = npos ) const -> String;

    // This constexpr implicit conversion function, which calls constexpr StringView(),
    // allows for String() == StringView()
    constexpr operator StringView() const noexcept { return StringView( mStr, mLen ); }

    static const int ssocapacity               { 20 };          // small string optimization allocates on stack
    static const int npos                      { -1 };          // mimicking the standard library
    char             mSSOBuffer[ ssocapacity ] { "" };
    char*            mStr                      { mSSOBuffer };
    int              mLen                      {};              // number of bytes before the null-terminator
    int              mCapacity                 { ssocapacity }; // includes the null-terminator (unlike std::string)
  };

  // -----------------------------------------------------------------------------------------------

  auto operator + ( char, const String& ) -> String;
  auto operator + ( const String&, char ) -> String;
  auto operator + ( const String&, const char* ) -> String;
  auto operator + ( const String&, const String& ) -> String;
  auto operator + ( const String&, StringView ) -> String;
  auto operator + ( const char*, const String& ) -> String;
  auto operator + ( const char*, StringView ) -> String;
  auto operator + ( StringView, const String& ) -> String;
  auto operator + ( StringView, const char* ) -> String;
  bool operator != ( const String& , const String&  );
  bool operator < ( const String& , const String&  );
  bool operator > ( const String& , const String&  );
  bool operator == ( StringView, StringView );
  bool operator == ( const String&, const String& );

} // namespace Tac

