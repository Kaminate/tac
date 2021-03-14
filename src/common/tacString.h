#pragma once

namespace Tac
{
  struct String;
  struct StringView;

  bool IsSpace( char );
  bool IsAlpha( char );

  int  StrLen( const char* );
  // Negative value if lhs appears before rhs in lexicographical order.
  // Zero if lhs and rhs compare equal.
  // Positive value if lhs appears after rhs in lexicographical order.
  int  StrCmp( const char*, const char* );
  int  MemCmp( const void*, const void*, int );

  //   std uses void*
  void MemCpy( void*, const void*, int );
  void StrCpy( char*, const char* );

  // so like if youre passing around data, i think stringview is nice because
  // your string doesnt have to be null terminated.
  struct StringView
  {
    StringView() = default;
    StringView( const char* );
    StringView( const char* , int );
    StringView( const char* strBegin, const char* strEnd );
    StringView( const String& );
    char operator[]( int i ) const;
    const char* data() const;
    int         size() const;
    const char* begin() const;
    const char* end() const;
    const char* c_str() const;
    bool        empty() const;
    operator const char* ( ){ return mStr; }

    //          searches this stringview for the last character which matches
    //          any of the characters in s, return npos if no matches
    int         find_last_of( StringView ) const;
    int         find_first_of( StringView ) const;
    StringView  substr( int pos = 0, int len = npos ) const;
    char        front();
    char        back();
    bool        starts_with( StringView ) const;
    bool        starts_with( char ) const;
    bool        ends_with( StringView ) const;
    void        remove_prefix( int );
    void        remove_suffix( int );

    static const int npos = -1; // mimicking the standard library
    const char*      mStr = nullptr;
    int              mLen = 0;
  };

  struct String
  {
    String();
    String( StringView rhs );
    String( const String& rhs );
    String( const char* begin, const char* end );
    String( const char* str, int len );
    String( int len, char c );
    String( const char* );
    ~String();
    char operator[]( int ) const;
    char& operator[]( int );
    void operator = ( StringView );
    void operator = ( const String& );
    void operator = ( const char* );
    void operator += ( const char* );
    void operator += ( const String& );
    void operator += ( char );
    operator const char* ( ){ return mStr; }
    void   clear();
    bool   empty() const;
    char*  c_str() const;
    int    size() const;
    char*  data() const;
    void   push_back( char );
    void   assign( const char* str, int strLen );
    void   append( const char* str, int strLen );
    void   append( const String& );
    void   prepend( const String& );
    void   reserve( int lenNotIncNull );
    void   resize( int lenNotIncNull );
    char*  begin() const;
    char*  end() const;
    //     returns npos if not found
    int    find_last_of( const char* c ) const;
    int    find( const String& substr ) const;
    String substr( int pos = 0, int len = npos ) const;

    static const int npos = -1; // mimicking the standard library
    char*  mStr = nullptr;
    int    mLen = 0; // number of bytes before the null-terminator
    int    mAllocatedByteCount = 0; // includes the null-terminator
  };
    
  //template <typename T> String ToString( T ){ return""; }
  //String     ToString<>( int ){}
  String     ToString( int );
  String     ToString( unsigned int );
  String     ToString( unsigned long long );
  String     ToString( void* );
  String     ToString( double );
  String     ToString( float );
  //String     ToString( uint32_t val );
  StringView Va( const char* format, ... );
  int        Atoi( StringView );

  // this is SeparateStrings in tacUtility.h
  //String Join( const String&, std::initializer_list< String > );
  //String Join( const String&, const String*, int );

  //template< typename T >
  //String Join( const String& sep, const T& ts )
  //{
  //  String result;
  //  for( int i = 0; i < ts.size(); ++i )
  //  {
  //    if( i )
  //    {
  //      result += sep;
  //    }
  //    result += ts[ i ];
  //  }
  //  return result;
  //}

  String operator + ( char, const String& );
  String operator + ( const String&, char );
  String operator + ( const String&, const char* );
  String operator + ( const String&, const String& );
  String operator + ( const char*, const String& );
  String operator + ( const char*, const StringView& );

  bool operator == ( const String& , const String&  );
  bool operator != ( const String& , const String&  );
  bool operator < ( const String& , const String&  );

  // For printing Tac::String to std::cout
  //std::ostream& operator << ( std::ostream& os, StringView s );
  //std::istream& operator >> ( std::istream& is, String& s );

  bool operator == ( const StringView&, const StringView& );
  bool operator == ( const StringView&, const String& );
  bool operator == ( const StringView&, const char* );
  bool operator == ( const String&, const StringView& );
  bool operator == ( const String&, const char* );

  struct StringID
  {
    StringID( StringView );
    static int Hash( StringView );
    int mHash = 0;
  };

  bool operator < ( StringID, StringID );
  bool operator == ( StringID, StringID );


  StringView DebugLookupString( StringID );
}
