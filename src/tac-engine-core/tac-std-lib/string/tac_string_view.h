#pragma once

namespace Tac
{

  // so like if youre passing around data, i think stringview is nice because
  // your string doesnt have to be null terminated.
  //
  // The problem is if you are passing a stringview to a function that expects a const char*
  // (for example a 3rd party library), you don't know if your underlying string is null-terminated
  // or not, so you have to pad it.
  //
  // Of course, if you are using const char*'s, and you need to take a substring, you need to
  // make another allocation to null terminate it.
  //
  // So it's kind of better to either use const char* everywhere, or StringView everywhere
  struct StringView
  {
    StringView() = default;
    StringView( const char* );
    //        StringView( const char*, int );
    constexpr StringView( const char* , int );
  //        StringView::StringView( const char* str, int len ) : mStr( str ), mLen( len ) {}

    constexpr StringView( const char* , const char* );
    //constexpr StringView( const char* strBegin, const char* strEnd );
    //StringView( const String& );
    //StringView( const StringLiteral& );
    //StringView( const ShortFixedString& );
    operator const char* ( ) const;
    char operator[]( int ) const;
    const char* data() const;
    int         size() const;
    const char* begin() const;
    const char* end() const;
    const char* c_str() const;
    bool        empty() const;

    //          searches this stringview for the last character which matches
    //          any of the characters in s, return npos if no matches
    int         find_last_of( const StringView& ) const;
    int         find_last_of( char ) const;
    int         find_first_of( const StringView& ) const;
    int         find_first_of( char ) const;

    //          returns the index of the substr, or npos
    int         find( const StringView& ) const;
    int         find( char ) const;
    bool        contains( const StringView& ) const;
    bool        contains( const char* ) const;
    bool        contains( char ) const;
    StringView  substr( int pos = 0, int len = npos ) const;
    char        front() const;
    char        back() const;
    bool        starts_with( StringView ) const;
    bool        starts_with( char ) const;
    bool        ends_with( StringView ) const;
    void        remove_prefix( int );
    void        remove_suffix( int );

    static const int npos = -1; // mimicking the standard library
    const char*      mStr = ""; // not nullptr, default should be printable
    int              mLen = 0;
  };


} // namespace Tac

constexpr Tac::StringView::StringView( const char* str, int len ) : mStr{ str }, mLen{ len } {}

constexpr Tac::StringView::StringView( const char* strBegin, const char* strEnd ) :
  mStr( strBegin ),
  mLen( ( int )( strEnd - strBegin ) )
{
}

