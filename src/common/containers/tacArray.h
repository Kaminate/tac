
// This file contains an array allocated on the sK
// Basically we would use a c-array for this,
// but having an arraysize macro is slightly less ugly
// than using templates
//
// Example use:
// auto foo = MakeArray< float >( 3.14f, 1337.0f );


#pragma once

#include "src/common/tacPreprocessor.h"

#include <utility> // std::forward
namespace Tac
{

  template< typename T, int N >
  struct Array
  {
    // Make this static?
    int size() { return N; }
    T* data() { return mTs; }
    T* begin() { return mTs; }
    T* end() { return mTs + N; }
    T& operator[]( int index ) { TAC_ASSERT( index >= 0 && index < N ); return mTs[ index ]; }
    const T& operator[] ( int index ) const { TAC_ASSERT( index >= 0 && index < N ); return mTs[ index ]; }
    T mTs[ N ];
  };

  //template< typename T, typename ... N >
  //constexpr auto MakeArray( N&&... values ) -> Array< T, sizeof...( values )>
  //{
  //  return { std::forward< T >( values )... };
  //}

  template< typename V, typename ... T >
  //constexpr auto MakeArray( T&&... values ) -> Array< std::common_type<T...>, sizeof...( values )>
  constexpr auto MakeArray( T&&... values ) -> Array< V, sizeof...( values )>
  {
    return { std::forward< T >( values )... };
  }





}



