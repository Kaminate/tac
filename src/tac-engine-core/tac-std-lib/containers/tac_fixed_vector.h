// A Tac::FixedVector is a cross between a Tac::Vector and a Tac::Array.
// Like a Tac::Vector, it has a variable size 
// Like a Tac::Array, it has a fixed capacity 

#pragma once

import std; // initializer list

//#include "tac-std-lib/preprocess/tac_preprocessor.h" // TAC_ASSERT
#include "tac-std-lib/error/tac_assert.h" // TAC_ASSERT

namespace Tac
{
  template< typename T, int N >
  struct FixedVector
  {
    FixedVector() = default;
    explicit FixedVector( int size ) : mTCount( size ) {}
    FixedVector( int size, T initialValue );
    FixedVector( T* tbegin, T* tend );
    FixedVector( std::initializer_list< T > );

    void     clear()                       { mTCount = 0; }
    int      size() const                  { return mTCount; }
    bool     empty() const                 { return !mTCount; }
    void     resize( int newSize )         { mTCount = newSize; }
    void     push_back( const T& t )       { TAC_ASSERT( mTCount < N ); mTs[ mTCount++ ] = t; }
    void     pop_back()                    { TAC_ASSERT( mTCount ); mTCount--; }
    void     append_range( const T*, int );

    T*       data()                        { return mTs; }
    const T* data() const                  { return mTs; }
    T&       front()                       { TAC_ASSERT( mTCount ); return mTs[ 0 ]; }
    T        front() const                 { TAC_ASSERT( mTCount ); return mTs[ 0 ]; }
    T        back()  const                 { TAC_ASSERT( mTCount ); return mTs[ mTCount - 1 ]; }
    T&       back()                        { TAC_ASSERT( mTCount ); return mTs[ mTCount - 1 ]; }
    T*       begin()                       { return mTs; }
    const T* begin() const                 { return mTs; }
    T*       end()                         { return mTs + mTCount; }
    const T* end()   const                 { return mTs + mTCount; }
    T&       operator[]( int index )       { return mTs[ index ]; }
    const T& operator[]( int index ) const { return mTs[ index ]; }

  private:
    T        mTs[ N ]{};
    int      mTCount = 0;
  };



  // -----------------------------------------------------------------------------------------------

  // Constructors


  template< typename T, int N >
  FixedVector<T, N>::FixedVector( int size, T initialValue )
  {
    resize( size );
    for( int i{}; i < size; ++i )
      mTs[ i ] = initialValue;
  }

  template< typename T, int N >
  FixedVector<T, N>::FixedVector( T* tbegin, T* tend )
  {
    int size = ( int )( tend - tbegin );
    resize( size );
    for( int i{}; i < size; ++i )
      mTs[ i ] = tbegin[ i ];
  }

  template< typename T, int N >
  FixedVector<T, N>::FixedVector( std::initializer_list< T > ts )
  {
    for( const T& t : ts )
      push_back( t );
  }

  // -----------------------------------------------------------------------------------------------

  // Member functions

  template< typename T, int N > void     FixedVector<T, N>::append_range( const T* ts, int n )
  {
    TAC_ASSERT( mTCount + n <= N );
    T* dst = end();
    mTCount += n;
    for( int i{}; i < n; ++i )
      *dst++ = *ts++;
  }

  // -----------------------------------------------------------------------------------------------

  // Out-of-class operators

  template<typename T, int N >
  bool operator ==( const FixedVector<T, N>& a, const FixedVector<T, N>& b )
  {
    const int n = a.size();
    if( n != b.size() )
      return false;

    for( int i{}; i < n; ++i )
      if( a[ i ] != b[ i ] )
        return false;

    return true;
  }

} // namespace Tac
