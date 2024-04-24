#pragma once

#include "tac-std-lib/string/tac_string_view.h"

namespace Tac { struct String; }

namespace Tac
{

  bool       IsSpace( char );
  bool       IsAlpha( char );
  bool       IsDigit( char );

  // Negative value if lhs appears before rhs in lexicographical order.
  // Zero if lhs and rhs compare equal.
  // Positive value if lhs appears after rhs in lexicographical order.
  int        StrCmp( const char*, const char* );
  void       StrCpy( char*, const char* );
  int        StrLen( const char* );

  int        MemCmp( const void*, const void*, int );
  void       MemCpy( void*, const void*, int );
  void       MemSet( void*, unsigned char, int );

  String     ToString( int );
  String     ToString( char );
  String     ToString( unsigned int );
  String     ToString( unsigned long long );
  String     ToString( const void* );
  String     ToString( double );
  String     ToString( float );

  // StringView Va( const char* format, ... );
  int        Atoi( const StringView& );
  float      Atof( const StringView& );
  String     Itoa( int, int base = 10 );

  // -----------------------------------------------------------------------------------------------

  // example:
  //
  //   u64 i = 123;
  //   printf( PRIu64, i ); 
  //   
  //   StringView s = "hello world";
  //   printf( TAC_PRI_SV_FMT, TAC_PRI_SV_ARG( s ) ); 

#define TAC_PRI_SV_FMT      "%.*s"
#define TAC_PRI_SV_ARG( s ) s.size(), s.data()

  // TODO: 
  //   in STL, the string capacity returns the number of characters that can be written
  //   to the string, not including the null terminator
  //   ie, buf_size = capacity + 1
  //   https://devblogs.microsoft.com/oldnewthing/20230803-00/?p=108532
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
    char operator[]( int ) const;
    char& operator[]( int );
    void operator = ( const String& );
    void operator = ( const StringView& );
    void operator = ( const char* );
    //void operator += ( const char* );
    void operator += ( const StringView& );
    //void operator += ( const String& );
    void operator += ( char );

   // stl doesnt have this, there is only operator string_view
    //operator const char* () const = delete;

    void   clear();
    bool   empty() const;
    const char*  c_str() const;
    const char*  data() const;
    int    size() const;
    void   erase( int pos, int len = npos );
    void   push_back( char );
    bool   starts_with( StringView ) const;
    bool   starts_with( char ) const;
    bool   ends_with( StringView ) const;
    void   assign( const char*, int );
    void   assign( const StringView& );
    void   append( const char*, int );
    void   append( const char* );
    void   append( const String& );
    void   prepend( const String& );
    void   reserve( int lenNotIncNull );
    void   resize( int lenNotIncNull );
    void   replace( StringView, StringView );
    void   pop_back();
    char*  begin() const;
    char*  end() const;
    char&  back();
    char&  front();
    int    compare( const char* ) const;
    //     returns npos if not found
    int    find_last_of( const char* c ) const;
    int    find( const String& substr ) const;
    int    find( char ) const;
    bool   contains( const StringView& ) const;
    bool   contains( char c ) const;
    String substr( int pos = 0, int len = npos ) const;

    // This constexpr implicit conversion function, which calls constexpr StringView(),
    // allows for String() == StringView()
    constexpr operator StringView() const noexcept { return StringView( mStr, mLen ); }

    static const int ssocapacity               { 20 };          // small string optimization allocates on stack
    static const int npos                      { -1 };          // mimicking the standard library
    char             mSSOBuffer[ ssocapacity ] { "" };
    char*            mStr                      { mSSOBuffer };
    int              mLen                      { 0 };           // number of bytes before the null-terminator
    int              mCapacity                 { ssocapacity }; // includes the null-terminator
  };

  // -----------------------------------------------------------------------------------------------

  // operator +
  String operator + ( char, const String& );
  String operator + ( const String&, char );
  String operator + ( const String&, const char* );
  String operator + ( const String&, const String& );
  String operator + ( const String&, const StringView& );
  String operator + ( const char*, const String& );
  String operator + ( const char*, const StringView& );
  String operator + ( const StringView&, const String& );
  String operator + ( const StringView&, const char* );

  // operator !=
  bool   operator != ( const String& , const String&  );

  // operator <
  bool   operator < ( const String& , const String&  );
  bool   operator > ( const String& , const String&  );

  // operator ==
  bool   operator == ( const StringView&, const StringView& );
  //bool   operator == ( const StringView&, const String& );
  //bool   operator == ( const StringView&, const char* );
  //bool   operator == ( const String&, const StringView& );
  bool   operator == ( const String&, const String& );
  //bool   operator == ( const String&, const char* );
  //bool   operator == ( const char*, const StringView& );



} // namespace Tac

