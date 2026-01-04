// A Tac::Array is basically a c-style array with member functions

#pragma once

#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

namespace Tac
{
  template< typename T, int N >
  struct Array
  {
    auto size() const -> int               { return N; }
    auto front() dynmc -> dynmc T&         { return mTs[ 0 ]; }
    auto front() const -> const T&         { return mTs[ 0 ]; }
    auto back() const -> const T&          { return mTs[ N - 1 ]; }
    auto back() dynmc -> dynmc T&          { return mTs[ N - 1 ]; }
    auto data() dynmc -> dynmc T*          { return mTs; }
    auto data() const -> const T*          { return mTs; }
    auto begin() dynmc -> dynmc T*         { return mTs; }
    auto begin() const -> const T*         { return mTs; }
    auto end() dynmc -> dynmc T*           { return mTs + N; }
    auto end() const -> const T*           { return mTs + N; }
    dynmc T& operator[]( int i ) dynmc     { TAC_ASSERT_INDEX( i, N ); return mTs[ i ]; }
    const T& operator[]( int i ) const     { TAC_ASSERT_INDEX( i, N ); return mTs[ i ]; }
    T mTs[ N ]{};
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



