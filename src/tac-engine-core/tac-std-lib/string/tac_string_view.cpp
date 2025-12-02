#include "tac_string_view.h" // self-inc

namespace Tac
{
  StringView::StringView( const char* str ) : mStr( str ), mLen( str ? StrLen( str ) : 0 ) { }
  StringView::operator const char* ( ) const                  { return mStr; }
  auto StringView::data() const -> const char*                { return mStr; }
  auto StringView::size() const -> int                        { return mLen; }
  auto StringView::begin() const -> const char*               { return mStr; }
  auto StringView::end() const -> const char*                 { return mStr + mLen; }
  auto StringView::c_str() const -> const char*               { return mStr; }
  bool StringView::empty() const                              { return mLen == 0; }
  auto StringView::find_last_of( const StringView s ) const -> int
  {
    for( int i{ mLen - 1 }; i >= 0; --i )
      if( s.contains( mStr[ i ] ) )
        return i;
    return npos;
  }
  auto StringView::find_last_of( char c ) const -> int
  {
    for( int i{ mLen - 1 }; i >= 0; --i )
      if( c == mStr[ i ] )
        return i;
    return npos;
  }
  auto StringView::find_first_of( char c ) const -> int
  {
    return find( c );
  }
  auto StringView::find_first_of( const StringView s ) const -> int
  {
    for( int i{}; i < mLen; ++i )
      if( s.contains( mStr[ i ] ) )
        return i;
    return npos;
  }
  auto StringView::find( const StringView substr ) const -> int
  {
    if( substr.mLen > mLen )
      return npos;
    for( int i{}; i <= mLen - substr.mLen; ++i )
      if( MemCmp( mStr + i, substr.mStr, substr.mLen ) == 0 )
        return i;
    return npos;
  }
  auto StringView::find( const char c ) const -> int
  {
    for( int i {}; i < mLen; ++i )
      if( c == mStr[ i ] )
        return i;
    return npos;
  }
  bool StringView::contains( const char* substr ) const       { return npos != find( substr ); }
  bool StringView::contains( const StringView substr ) const { return npos != find( substr ); }
  bool StringView::contains( char c ) const                   { return npos != find( c ); }
  auto StringView::substr( const int pos, const int len ) const -> StringView
  {
    const int iEnd = len == npos ? mLen: pos + len;
    TAC_ASSERT( pos >= 0 && pos <= mLen );
    TAC_ASSERT( iEnd >= 0 && iEnd <= mLen );
    TAC_ASSERT( pos <= iEnd );
    return StringView( mStr + pos, mStr + iEnd );
  }
  char StringView::front() const                              { return mStr[ 0 ]; }
  char StringView::back() const                               { return mStr[ mLen - 1 ]; }
  void StringView::remove_prefix( const int n )
  {
    mStr += n;
    mLen -= n;
  }
  void StringView::remove_suffix( const int n )
  {
    mLen -= n;
  }
  bool StringView::starts_with( StringView s ) const
  {
    return mLen >= s.mLen && MemCmp( mStr, s.mStr, s.mLen ) == 0;
  }
  bool StringView::starts_with( char c ) const
  {
    return mLen && mStr[ 0 ] == c;
  }
  bool StringView::ends_with( StringView s ) const
  {
    return mLen >= s.mLen && MemCmp( mStr + mLen - s.mLen, s.mStr, s.mLen ) == 0;
  }
  char StringView::operator[]( int i ) const                  { return mStr[ i ]; }
}

