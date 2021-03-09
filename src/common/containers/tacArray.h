
// This file contains an array allocated on the sK
// Basically we would use a c-array for this,
// but having an arraysize macro is slightly less ugly
// than using templates
//
// Example use:
// auto foo = MakeArray< float >( 3.14f, 1337.0f );


#pragma once

namespace Tac
{

  template< typename T, int N >
  struct Array
  {
    // Make this static?
    int      size()  const { return N; }
    T        front() const { return mTs[ 0 ]; }
    T        back()  const { return mTs[ N - 1 ]; }
    T*       data()        { return mTs; }
    T*       begin()       { return mTs; }
    const T* begin() const { return mTs; }
    T*       end()         { return mTs + N; }
    const T* end()   const { return mTs + N; }
    T&       operator[]( int index )       {  return mTs[ index ]; }
    const T& operator[]( int index ) const {  return mTs[ index ]; }
    T        mTs[ N ];
  };
}



