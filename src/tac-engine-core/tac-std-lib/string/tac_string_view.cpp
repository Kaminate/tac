#include "tac_string_view.h" // self-inc

// -------------------------------------------------------------------------------------------------

namespace Tac
{

  StringView::StringView( const char* str ) : mStr( str ), mLen( str ? StrLen( str ) : 0 ) { }

  //constexpr StringView::StringView( const char* str, int len ) : mStr( str ), mLen( len ) {}
  //        StringView::StringView( const char* str, int len ) : mStr( str ), mLen( len ) {}

  //constexpr StringView::StringView( const char* strBegin, const char* strEnd ) :
  //  mStr( strBegin ),
  //  mLen( ( int )( strEnd - strBegin ) )
  //{
  //}

  //StringView::StringView( const String& str ) : mStr ( str.mStr ), mLen ( str.mLen ) { }

  //StringView::StringView( const StringLiteral& str ) : StringView( str.c_str() ) {}

  //StringView::StringView( const ShortFixedString& s ) : StringView( s.data(), s.size() ) {}

  StringView::operator const char* ( ) const        { return mStr; }
  char        StringView::operator[]( int i ) const { return mStr[ i ]; }
  const char* StringView::data() const              { return mStr; }
  int         StringView::size() const              { return mLen; }
  const char* StringView::begin() const             { return mStr; }
  const char* StringView::end() const               { return mStr + mLen; }
  const char* StringView::c_str() const             { return mStr; }
  bool        StringView::empty() const             { return mLen == 0; }
  int         StringView::find_last_of( const StringView& s ) const
  {
    for( int i{ mLen - 1 }; i >= 0; --i )
      if( s.contains( mStr[ i ] ) )
        return i;
    return npos;
  }

  int         StringView::find_last_of( char c ) const
  {
    for( int i{ mLen - 1 }; i >= 0; --i )
      if( c == mStr[ i ] )
        return i;
    return npos;
  }

  int         StringView::find_first_of( char c ) const
  {
    return find( c );
  }

  int         StringView::find_first_of( const StringView& s ) const
  {
    for( int i{}; i < mLen; ++i )
      if( s.contains( mStr[ i ] ) )
        return i;
    return npos;
  }
  int         StringView::find( const StringView& substr ) const
  {
    if( substr.mLen > mLen )
      return npos;
    for( int i{}; i <= mLen - substr.mLen; ++i )
      if( MemCmp( mStr + i, substr.mStr, substr.mLen ) == 0 )
        return i;
    return npos;
  }

  int         StringView::find( const char c ) const
  {
    for( int i {}; i < mLen; ++i )
      if( c == mStr[ i ] )
        return i;
    return npos;
  }

  bool        StringView::contains( const char* substr ) const       { return npos != find( substr ); }
  bool        StringView::contains( const StringView& substr ) const { return npos != find( substr ); }
  bool        StringView::contains( char c ) const                   { return npos != find( c ); }

  StringView  StringView::substr( const int pos, const int len ) const
  {
    const int iEnd = len == npos ? mLen: pos + len;
    TAC_ASSERT( pos >= 0 && pos <= mLen );
    TAC_ASSERT( iEnd >= 0 && iEnd <= mLen );
    TAC_ASSERT( pos <= iEnd );
    return StringView( mStr + pos, mStr + iEnd );
  }

  char        StringView::front() const { return mStr[ 0 ]; }
  char        StringView::back() const { return mStr[ mLen - 1 ]; }
  void        StringView::remove_prefix( const int n )
  {
    mStr += n;
    mLen -= n;
  }
  void        StringView::remove_suffix( const int n )
  {
    mLen -= n;
  }
  bool        StringView::starts_with( StringView s ) const
  {
    return mLen >= s.mLen && MemCmp( mStr, s.mStr, s.mLen ) == 0;
  }
  bool        StringView::starts_with( char c ) const
  {
    return mLen && mStr[ 0 ] == c;
  }
  bool        StringView::ends_with( StringView s ) const
  {
    return mLen >= s.mLen && MemCmp( mStr + mLen - s.mLen, s.mStr, s.mLen ) == 0;
  }

}

