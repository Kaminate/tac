#pragma once
#include <cstdint>

namespace Tac
{
  typedef uint32_t HashedValue;

  HashedValue HashAddBytes( HashedValue, const char*, int );
  HashedValue HashAddHash( HashedValue, HashedValue );
  template< typename T >
  HashedValue HashAdd( HashedValue hashedValue, T t ) { return HashAddHash( hashedValue, ( HashedValue )t ); }

}
