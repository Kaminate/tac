#pragma once
#include <cstdint>

namespace Tac
{
  typedef uint32_t HashedValue;

  HashedValue HashAddString( const char*, HashedValue = 0);
  HashedValue HashAddBytes( const char*, int, HashedValue = 0 );
  HashedValue HashAddHash( HashedValue, HashedValue = 0 );

  // Used to hash integral types
  template< typename T > HashedValue HashAdd( T t, HashedValue h = 0 )
  {
    return HashAddHash( ( HashedValue )t, h );
  }

}
