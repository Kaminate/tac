#include "src/common/tacHash.h"

namespace Tac
{
  HashedValue HashAddBytes( HashedValue hashedValue,  const char* bytes, int byteCount  )
  {
    for( int iByte = 0; iByte < byteCount; ++iByte )
      hashedValue = HashAdd( hashedValue, bytes[ iByte ] );
    return hashedValue;
  }

  HashedValue HashAddHash( HashedValue a, HashedValue b)
  {
    return 37 * a + b;
  }
}
