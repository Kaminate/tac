// This file contains an array allocated on the stack
// Basically we would use a c-array for this,
// but having an arraysize macro is slightly less ugly
// than using templates
//
// Example use:
// auto foo = TacMakeArray< float >( 3.14f, 1337.0f );
 

#pragma once

#include "common/tacPreprocessor.h"

#include <utility> // std::forward

template< typename T, int N >
struct TacArray
{
    // Make this static?
  int size() { return N; }
  T* data() { return mTs; }
  T* begin() { return mTs; }
  T* end() { return mTs + N; }
  T& operator[]( int index ) { TacAssert( index >= 0 && index < N ); return mTs[ index ]; }
  const T& operator[] (int index) const{ TacAssert(index >= 0 && index < N); return mTs[index]; }
  T mTs[ N ];
};

//template< typename T, typename ... N >
//constexpr auto TacMakeArray( N&&... values ) -> TacArray< T, sizeof...( values )>
//{
//  return { std::forward< T >( values )... };
//}

template< typename V, typename ... T >
//constexpr auto TacMakeArray( T&&... values ) -> TacArray< std::common_type<T...>, sizeof...( values )>
constexpr auto TacMakeArray( T&&... values ) -> TacArray< V, sizeof...( values )>
{
  return { std::forward< T >( values )... };
}








