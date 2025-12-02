#include "tac_hash.h" // self-inc

//#include "tac-std-lib/containers/tac_array.h"
//#include "tac-engine-core/asset/tac_asset.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"

namespace Tac
{
  // -----------------------------------------------------------------------------------------------

  static HashValue HashCombine( HashValue a, HashValue b )
  {
    return 37 * a + b;
  }

}

  // -----------------------------------------------------------------------------------------------

  Tac::HashValue Tac::Hashu32( u32 val )
  {
    return 37 * val;
  }

  Tac::HashValue Tac::Hashu64( u64 val )
  {
    const HashValue h0{ ( HashValue )( val >> 4 ) };
    const HashValue h1{ ( HashValue )( val & 0xff ) };
    return Hash( h0, h1 );
  }

  Tac::HashValue Tac::Hash( int val )
  {
    return ( HashValue )( 37 * val );
  }

  Tac::HashValue Tac::Hash( u64 val )
  {
    return Hashu64( val );
  }

  Tac::HashValue Tac::Hash( const char* s )
  {
    Hasher h;
    while( char c = *s++ )
      h.Eat( c );
    return h;
  }

  Tac::HashValue Tac::Hash( const char* s, int n)
  {
    Hasher h;
    for( int i{}; i < n; ++i )
      h.Eat( s[ i ] );
    return h;
  }

  //HashValue Hash( int i )
  //{
  //  return Hash( ( HashValue )i );
  //}

  Tac::HashValue Tac::Hash( const StringView s) { return Hash( s.data(), s.size() ); }
  Tac::HashValue Tac::Hash( const String& s)     { return Hash( s.data(), s.size() ); }

  Tac::HashValue Tac::Hash( HashValue* v, int n)
  {
    Hasher h;
    for( int i{}; i < n; ++i )
      h.Eat( v[i] );
    return h;
  }

  Tac::HashValue Tac::Hash( HashValue a )
  {
    // you know, if our input is already hashed, there's no point in hashing it further.
    // but if our input is just a normal u32, then we should hash it, right?
    //
    // so maybe HashValue should be a struct{ u32 } instead of a typedef, so that we can
    // differentiate hashed vs unhashed u32s.
    return a;
  }

  Tac::HashValue Tac::Hash( HashValue a, HashValue b)
  {
    return HashCombine( a, b );
  }

  Tac::HashValue Tac::Hash( HashValue a, HashValue b, HashValue c )
  {
    Hasher h;
    h.Eat( a );
    h.Eat( b );
    h.Eat( c );
    return h;
  }

  Tac::HashValue Tac::Hash( HashValue a, HashValue b, HashValue c, HashValue d )
  {
    Hasher h;
    h.Eat( a );
    h.Eat( b );
    h.Eat( c );
    h.Eat( d );
    return h;
  }

  Tac::HashValue Tac::Hash( HashValue a, HashValue b, HashValue c, HashValue d, HashValue e)
  {
    Hasher h;
    h.Eat( a );
    h.Eat( b );
    h.Eat( c );
    h.Eat( d );
    h.Eat( e );
    return h;
  }

  namespace Tac
  {

  // -----------------------------------------------------------------------------------------------
  void Hasher::Eat( HashValue v )
  {
    mHashValue = HashCombine( mHashValue, v );
  }

  Hasher::operator HashValue() const
  {
    return mHashValue;
  }
  // -----------------------------------------------------------------------------------------------

  // this shouldnt exist here?
  //HashValue Hash( const AssetPathString& s)
  //{
  //  return Hash( s.data(), s.size() );
  //}

} // namespace Tac
