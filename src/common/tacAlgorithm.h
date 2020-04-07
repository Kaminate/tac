
// This file contains common operations that you would do on generic types
// Our version of stl's #include <algorithm>

#pragma once

#include "src/common/containers/tacVector.h"
#include "src/common/containers/tacArray.h"


#include <set>
#include <map>
#include <initializer_list>

namespace Tac
{

// ie: if( Contains( { 1, 2, 3, 4, 5, 6 }, i )
template< typename T, typename U >
bool Contains( const T& elements, const U& element )
{
  return std::find( elements.begin(), elements.end(), element ) != elements.end();
}
template< typename U >
bool Contains( const std::set< U>& elements, const U& element )
{
  return elements.find( element ) != elements.end();
}
template< typename T, typename U >
bool Contains( const std::map< T, U >& elements, const T& key )
{
  return elements.find( key ) != elements.end();
}

// ie: int i = Random( { 1, 2, 3, 4, 5, 6 } )
template< typename T >
auto RandomAccess( const T& ts ) -> decltype( ts[ 0 ] )
{
  return ts[ std::rand() % ( int )ts.size() ];
}
template< typename T >
auto RandomNoAccess( const T& ts ) -> decltype( *ts.begin() )
{
  int c = std::rand() % ts.size();
  auto it = ts.begin();
  for( int i = 0; i < c; ++i )
    ++it;
  return *it;
}
template< typename T >
auto Random( const T& ts ) -> decltype( *ts.begin() )
{
  int c = std::rand() % ( int )ts.size();
  auto it = ts.begin();
  for( int i = 0; i < c; ++i )
    ++it;
  return *it;
}
template< typename T > T Random( const Vector< T >& ts ) { return RandomAccess( ts ); }
template< typename T, size_t U > T Random( const std::array< T, U >& ts ) { return RandomAccess( ts ); }
template< typename T > T Random( std::initializer_list<T> ts ) { return RandomNoAccess( ts ); }

template< typename T >
void Swap( T& a, T& b )
{
  T temp = a;
  a = b;
  b = temp;
}


}

