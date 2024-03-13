#include "tac-std-lib/dataprocess/tac_hash.h" // self-inc

//#include "tac-std-lib/containers/tac_array.h"
//#include "tac-std-lib/filesystem/tac_asset.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"

namespace Tac
{
  // -----------------------------------------------------------------------------------------------

  static HashValue HashCombine( HashValue a, HashValue b )
  {
    return 37 * a + b;
  }

  // -----------------------------------------------------------------------------------------------

  HashValue Hashu32( u32 val )
  {
    return 37 * val;
  }

  HashValue Hashu64( u64 val )
  {
    const union { u64 mU64; u32 mU32[ 2 ]; } u{ .mU64 = val };
    return Hashu32( u.mU32[ 0 ] ) + Hashu32( u.mU32[ 1 ] );
  }

  HashValue Hash( u64 val )
  {
    return Hashu64( val );
  }

  HashValue Hash( const char* s )
  {
    Hasher h;
    while( char c = *s++ )
      h.Eat( c );
    return h;
  }

  HashValue Hash( const char* s, int n)
  {
    Hasher h;
    for( int i = 0; i < n; ++i )
      h.Eat( s[ i ] );
    return h;
  }

  //HashValue Hash( int i )
  //{
  //  return Hash( ( HashValue )i );
  //}

  HashValue Hash( const StringView& s) { return Hash( s.data(), s.size() ); }
  HashValue Hash( const String& s)     { return Hash( s.data(), s.size() ); }

  HashValue Hash( HashValue* v, int n)
  {
    Hasher h;
    for( int i = 0; i < n; ++i )
      h.Eat( v[i] );
    return h;
  }

  HashValue Hash( HashValue a )
  {
    // you know, if our input is already hashed, there's no point in hashing it further.
    // but if our input is just a normal u32, then we should hash it, right?
    //
    // so maybe HashValue should be a struct{ u32 } instead of a typedef, so that we can
    // differentiate hashed vs unhashed u32s.
    return a;
  }

  HashValue Hash( HashValue a, HashValue b)
  {
    return HashCombine( a, b );
  }

  HashValue Hash( HashValue a, HashValue b, HashValue c )
  {
    Hasher h;
    h.Eat( a );
    h.Eat( b );
    h.Eat( c );
    return h;
  }

  HashValue Hash( HashValue a, HashValue b, HashValue c, HashValue d )
  {
    Hasher h;
    h.Eat( a );
    h.Eat( b );
    h.Eat( c );
    h.Eat( d );
    return h;
  }

  HashValue Hash( HashValue a, HashValue b, HashValue c, HashValue d, HashValue e)
  {
    Hasher h;
    h.Eat( a );
    h.Eat( b );
    h.Eat( c );
    h.Eat( d );
    h.Eat( e );
    return h;
  }

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
