// A Tac::Array is basically a c-style array with member functions

#pragma once

#include "tac-std-lib/error/tac_assert.h"

namespace Tac
{
  template< typename T, int N >
  struct Array
  {
    int      size()  const                 { return N; }
    T&       front()                       { return mTs[ 0 ]; }
    T        front() const                 { return mTs[ 0 ]; }
    T        back()  const                 { return mTs[ N - 1 ]; }
    T&       back()                        { return mTs[ N - 1 ]; }
    T*       data()                        { return mTs; }
    const T* data() const                  { return mTs; }
    T*       begin()                       { return mTs; }
    const T* begin() const                 { return mTs; }
    T*       end()                         { return mTs + N; }
    const T* end()   const                 { return mTs + N; }
    T&       operator[]( int i )           { TAC_ASSERT_INDEX( i, N ); return mTs[ i ]; }
    const T& operator[]( int i ) const     { TAC_ASSERT_INDEX( i, N ); return mTs[ i ]; }
    T        mTs[ N ]{};
  };

  // C++17 array deduction guide. Tac::Array foo{ 2, 4, 6 }; is deduced to be Array< int, 3 >
  template< class T, class... U > Array( T, U... )->Array< T, 1 + sizeof...( U ) >;

  template< typename T, int N > bool operator == ( const Array<T, N>& a, const Array<T, N>& b )
  {
    for( int i{}; i < N; ++i )
      if( a[ i ] != b[ i ] )
        return false;
      return true;
  }

#if 0
  template< typename T, int N > bool operator != ( const Array<T, N>& a, const Array<T, N>& b )
  {
    return !( a == b );
  }
#endif

} // namespace Tac



