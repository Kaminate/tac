// This file contains common operations that you would do on generic types
// Our version of stl's #include <algorithm>
#pragma once

// #include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/math/tac_math.h" // RandomIndex
//#include "tac-std-lib/containers/tac_array.h"

// remove these?
//#include <cstdlib> // std::rand()
//#include <array> // std::array()

namespace Tac
{

  // ie: if( Contains( { 1, 2, 3, 4, 5, 6 }, i )
  template< typename T, typename U > bool Contains( const T& elements, const U& element )
  {
    auto beginIt { elements.begin() };
    auto endIt { elements.end() };
    for( auto it { beginIt }; it != endIt; ++it )
      if( *it == element )
        return true;
    return false;
  }

  template< typename T> bool              Contains( const T* beginElement, const T* endElement, const T& element )
  {
    for( const T* curElement { beginElement }; curElement != endElement; ++curElement )
      if( *curElement == element )
        return true;
    return false;
  }

  // ie: int i = Random( { 1, 2, 3, 4, 5, 6 } )
  template< typename T > auto             RandomAccess( const T& ts ) -> decltype( ts[ 0 ] )
  {
    return ts[ RandomIndex( ( int )ts.size() ) ];
  }

  template< typename T > auto             RandomNoAccess( const T& ts ) -> decltype( *ts.begin() )
  {
    int c = RandomIndex( ts.size() );
    auto it = ts.begin();
    for( int i{}; i < c; ++i )
      ++it;
    return *it;
  }

  //template< typename T > auto             Random( const T& ts ) -> decltype( *ts.begin() )
  //{
  //  int c = RandomIndex( ( int )ts.size() );
  //  auto it = ts.begin();
  //  for( int i{}; i < c; ++i )
  //    ++it;
  //  return *it;
  //}

  //template< typename T > T                Random( const Vector< T >& ts )        { return RandomAccess( ts );   }
  //template< typename T, size_t U > T      Random( const std::array< T, U >& ts ) { return RandomAccess( ts );   }
  //template< typename T > T                Random( std::initializer_list<T> ts )  { return RandomNoAccess( ts ); }

  template< typename T > void             Swap( T& a, T& b )                    
  {
    T temp{ ( T&& )a };
    a = ( T&& )b;
    b = ( T&& )temp;
  }

  template< typename T > int              Count( const T& ts, decltype ( *ts.begin() ) value )
  {
    int n {};
    for( auto t : ts )
      if( t == value )
        ++n;
    return n;
  }

  // this could be templated
  template< typename T > void             Reverse( T* begin, T* end )
  {
    --end;
    while( begin < end )
    {
      Swap( *begin, *end );
      ++begin;
      --end;
    }
  }

  template< typename TIter, typename T >
  TIter Find( TIter first, TIter last, const T& t )
  {
    for( TIter iter { first }; iter != last; ++iter )
      if( *iter == t )
        return iter;
    return last;
  }

  template< typename TIter, typename TPred >
  TIter FindIf( TIter first, TIter last, TPred pred )
  {
    for( TIter iter { first }; iter != last; ++iter )
      if( pred( *iter ) )
        return iter;
    return last;
  }

} // namespace Tac

