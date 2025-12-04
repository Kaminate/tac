// A Tac::FixedVector is a cross between a Tac::Vector and a Tac::Array.
// Like a Tac::Vector, it has a variable size 
// Like a Tac::Array, it has a fixed capacity 

#pragma once


#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/error/tac_assert.h" // TAC_ASSERT

#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <initializer_list>
#endif

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

    void clear()                                   { mTCount = 0; }
    auto size() const -> int                       { return mTCount; }
    bool empty() const                             { return !mTCount; }
    void resize( int newSize )                     { mTCount = newSize; }
    void push_back( const T& t )                   { TAC_ASSERT( mTCount < N ); mTs[ mTCount++ ] = t; }
    void pop_back()                                { TAC_ASSERT( mTCount ); mTCount--; }
    void append_range( const T*, int );
    auto max_size() const -> int                   { return N; }
    auto data() dynmc -> dynmc T*                  { return mTs; }
    auto data() const -> const T*                  { return mTs; }
    auto front() dynmc -> dynmc T&                 { TAC_ASSERT( mTCount ); return mTs[ 0 ]; }
    auto front() const -> const T&                 { TAC_ASSERT( mTCount ); return mTs[ 0 ]; }
    auto back() const -> const T&                  { TAC_ASSERT( mTCount ); return mTs[ mTCount - 1 ]; }
    auto back() dynmc -> dynmc T&                  { TAC_ASSERT( mTCount ); return mTs[ mTCount - 1 ]; }
    auto begin() dynmc -> dynmc T*                 { return mTs; }
    auto begin() const -> const T*                 { return mTs; }
    auto end() dynmc -> dynmc T*                   { return mTs + mTCount; }
    auto end() const -> const T*                   { return mTs + mTCount; }
    auto operator[]( int index ) dynmc -> dynmc T& { return mTs[ index ]; }
    auto operator[]( int index ) const -> const T& { return mTs[ index ]; }

  private:
    T        mTs[ N ]{};
    int      mTCount {};
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

  template< typename T, int N >
  void FixedVector<T, N>::append_range( const T* ts, int n )
  {
    TAC_ASSERT( mTCount + n <= N );
    T* dst { end() };
    mTCount += n;
    for( int i{}; i < n; ++i )
      *dst++ = *ts++;
  }

  // -----------------------------------------------------------------------------------------------

  // Out-of-class operators

  template<typename T, int N >
  bool operator ==( const FixedVector<T, N>& a, const FixedVector<T, N>& b )
  {
    const int n { a.size() };
    if( n != b.size() )
      return false;

    for( int i{}; i < n; ++i )
      if( a[ i ] != b[ i ] )
        return false;

    return true;
  }

} // namespace Tac
