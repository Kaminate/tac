#pragma once
#include <cstdint>

namespace Tac
{
  typedef uint32_t HashedValue;

  //HashedValue HashString( const char* );
  //HashedValue HashAddBytes( const char*, int );
  HashedValue HashAddString( const char*, HashedValue = 0);
  HashedValue HashAddBytes( const char*, int, HashedValue = 0 );
  HashedValue HashAddHash( HashedValue, HashedValue );
  template< typename T > HashedValue HashAdd( HashedValue h, T t ) { return HashAddHash( h, ( HashedValue )t ); }

}
