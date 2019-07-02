// This file contains common operations that you would do on generic types
// Our version of stl's #include <algorithm>

#pragma once

#include "common/containers/tacVector.h"
#include "common/containers/tacArray.h"


#include <set>
#include <map>
#include <initializer_list>


// ie: if( TacContains( { 1, 2, 3, 4, 5, 6 }, i )
template< typename T, typename U >
bool TacContains( const T& elements, const U& element )
{
  return std::find( elements.begin(), elements.end(), element ) != elements.end();
}
template< typename U >
bool TacContains( const std::set< U>& elements, const U& element )
{
  return elements.find( element ) != elements.end();
}
template< typename T, typename U >
bool TacContains( const std::map< T, U >& elements, const T& key )
{
  return elements.find( key ) != elements.end();
}

// ie: int i = TacRandom( { 1, 2, 3, 4, 5, 6 } )
template< typename T >
auto TacRandomAccess( const T& ts ) -> decltype( ts[ 0 ] )
{
  return ts[ std::rand() % ( int )ts.size() ];
}
template< typename T >
auto TacRandomNoAccess( const T& ts ) -> decltype( *ts.begin() )
{
  int c = std::rand() % ts.size();
  auto it = ts.begin();
  for( int i = 0; i < c; ++i )
    ++it;
  return *it;
}
template< typename T >
auto TacRandom( const T& ts ) -> decltype( *ts.begin() )
{
  int c = std::rand() % ( int )ts.size();
  auto it = ts.begin();
  for( int i = 0; i < c; ++i )
    ++it;
  return *it;
}
template< typename T > T TacRandom( const TacVector< T >& ts ) { return TacRandomAccess( ts ); }
template< typename T, size_t U > T TacRandom( const std::array< T, U >& ts ) { return TacRandomAccess( ts ); }
template< typename T > T TacRandom( std::initializer_list<T> ts ) { return TacRandomNoAccess( ts ); }

template< typename TElement, typename TContainer >
int TacIndexOf( const TElement& element, const TContainer& container )
{
  int i = 0;
  for( const TElement& curElement : container )
  {
    if( curElement == element )
    {
      break;
    }
    i++;
  }
  return i;
}

// Not sure if this will be that useful...
// A lot of the time, you find an element from a key,
// do some stuff with it, and then remove it from the container.
// std::find_if returns an iterator that you can then use to remove the element
template< typename TElement, typename TContainer, typename TPredicate >
bool TacFindIf( TElement* foundElement, TContainer& elements, TPredicate predicate )
{
  for( TElement& currentElement : elements )
  {
    if( predicate( currentElement ) )
    {
      *foundElement = currentElement;
      return true;
    }
  }
  return false;
}

template< typename T >
void TacSwap( T& a, T& b )
{
  T temp = a;
  a = b;
  b = temp;
}

template< typename T >
void TacUnorderedVectorRemove( TacVector< T >& container, int indexToRemove )
{
  container[ indexToRemove ] = container[ container.size() - 1 ];
  container.pop_back();
}
