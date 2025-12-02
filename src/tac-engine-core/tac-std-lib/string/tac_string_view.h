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
    StringView( int ) = delete; // prevent StringView( 0 ) from compiling
    constexpr StringView( const char*, int );
    constexpr StringView( const char*, const char* );
    operator const char* () const;
    auto data() const -> const char*;
    auto size() const -> int;
    auto begin() const -> const char*;
    auto end() const -> const char*;
    auto c_str() const -> const char*;
    bool empty() const;

    //   searches this stringview for the last character which matches
    //   any of the characters in s, return npos if no matches
    auto find_last_of( StringView ) const -> int;
    auto find_last_of( char ) const -> int;
    auto find_first_of( StringView ) const -> int;
    auto find_first_of( char ) const -> int;

    auto find( StringView ) const -> int; // returns the index of the substr, or npos
    auto find( char ) const -> int;
    bool contains( StringView ) const;
    bool contains( const char* ) const;
    bool contains( char ) const;
    auto substr( int pos = 0, int len = npos ) const -> StringView;
    char front() const;
    char back() const;
    bool starts_with( StringView ) const;
    bool starts_with( char ) const;
    bool ends_with( StringView ) const;
    void remove_prefix( int );
    void remove_suffix( int );
    char operator[]( int ) const;

    static const int npos { -1 }; // mimicking the standard library
    const char*      mStr { "" }; // not nullptr, default should be printable
    int              mLen {};
  };


} // namespace Tac

constexpr Tac::StringView::StringView( const char* str, int len ) : mStr{ str }, mLen{ len } {}

constexpr Tac::StringView::StringView( const char* strBegin, const char* strEnd ) :
  mStr( strBegin ),
  mLen( ( int )( strEnd - strBegin ) )
{
}

