#include "src/common/tacHash.h"

namespace Tac
{
  //HashedValue HashString( const char* bytes )
  //{
  //  HashedValue result = 0;
  //  while( char c = *bytes++ )
  //    result = HashAddHash( result, ( HashedValue )c );
  //  return result;
  //}
  
  //HashedValue HashAddBytes( const char* bytes, int byteCount)
  //{
  //  return HashAddBytes( 0, bytes, byteCount );
  //}

  HashedValue HashAddString(  const char* bytes ,HashedValue hashedValue)
  {
    while( char c = *bytes++ )
      hashedValue = HashAddHash( hashedValue, ( HashedValue )c );
    return hashedValue;
  }

  HashedValue HashAddBytes(   const char* bytes, int byteCount  ,HashedValue hashedValue)
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
