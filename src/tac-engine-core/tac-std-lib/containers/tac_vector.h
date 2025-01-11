#pragma once

#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/algorithm/tac_algorithm.h" // Swap

#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <initializer_list>
#endif

namespace Tac
{

  // How come ~T() is never called?
  // Seems like a mega bug

  template< typename T >
  struct Vector
  {
    Vector() = default;

    Vector( Vector&& v )
    {
      mTs = v.mTs;
      mTCount = v.mTCount;
      mTCapacity = v.mTCapacity;
      v.mTs = {};
      v.mTCount = {};
      v.mTCapacity = {};
    }

    Vector( const Vector& v )
    {
      const int size { v.size() };
      resize( size );
      for( int i{}; i < size; ++i )
        mTs[ i ] = v[ i ];
    }

    Vector( int size ) { resize( size ); }

    Vector( int size, T initialValue )
    {
      assign( size, initialValue );
    }

    Vector( const T* tbegin, const T* tend )
    {
      const int size { ( int )( tend - tbegin ) };
      resize( size );
      for( int i{}; i < size; ++i )
        mTs[ i ] = tbegin[ i ];
    }

    Vector( std::initializer_list< T > ts )
    {
      assign( ts.begin(), ts.end() );
    }

    ~Vector()
    {
      clear();
      Deallocate( mTs );
      mTs = {};
      mTCapacity = {};
    }

    void     operator =( Vector< T >&& v )
    {
      const int n{ v.size() };
      resize( n );
      for( int i{}; i < n; ++i )
      {
        Swap( ( *this )[ i ], v[ i ] );
      }
      // 
    }

    void     operator =( const Vector< T >& v )
    {
      assign( v.begin(), v.end() );
    }

    void     assign( const T* begin, const T* end )
    {
      const int n { ( int )( end - begin ) };
      resize( n );
      for( int i{}; i < n; ++i )
        mTs[ i ] = begin[ i ];
    }

    void     assign( int n, const T& t )
    {
      resize( n );
      for( int i{}; i < n; ++i )
        mTs[ i ] = t;
    }

    void     clear()
    {
      for( int i{}; i < mTCount; ++i )
        mTs[ i ].~T();

      mTCount = 0;
    }

    void     push_back( T&& t )
    {
      reserve();
      T* dst{ &mTs[ mTCount++ ] };
      TAC_PLACEMENT_NEW( dst )T( move( t ) );
    }

    void     push_back( const T& t )
    {
      reserve();
      T* dst{ &mTs[ mTCount++ ] };
      TAC_PLACEMENT_NEW( dst )T( t );
    }

    template< class ... Args >
    T&       emplace_back( Args&& ... args )
    {
      reserve();
      TAC_PLACEMENT_NEW( &mTs[ mTCount++ ] )T( forward< Args>( args )... );
      return back();
    }

    void     pop_back()
    {
      TAC_ASSERT( mTCount );
      mTs[ mTCount - 1 ].~T();
      mTCount--;
    }
    bool     empty() const              { return !mTCount; }
    int      size() const               { return mTCount; }

    void     resize( const int newSize )
    {
      const int oldSize{ mTCount };
      reserve( newSize );

      if( newSize > oldSize )
      {
        for( int i{ oldSize }; i < newSize; ++i )
        {
          TAC_PLACEMENT_NEW( &mTs[ i ] )T();
        }
      }

      if( newSize < oldSize )
      {
        for( int i{ newSize }; i < oldSize; ++i )
        {
          mTs[i].~T();
        }
      }

      mTCount = newSize;
    }

    void     resize( int newSize, const T value )
    {
      const int oldSize{ mTCount };
      resize( newSize );

      for( int i{ oldSize }; i < newSize; ++i )
        mTs[ i ] = value;

      mTCount = newSize;
    }

    void reserve()
    {
      if( mTCount == mTCapacity )
        reserve( int( mTCount * 1.5f ) + 1 );
    }

    void     reserve( int capacity )
    {
      if( capacity <= mTCapacity )
        return;

      T* newTs{ ( T* )Allocate( sizeof( T ) * capacity ) };
      for( int i{}; i < mTCount; ++i )
        TAC_PLACEMENT_NEW ( &newTs[ i ] )T( Tac::move( mTs[ i ] ) );

      Deallocate( mTs );
      mTs = newTs;
      mTCapacity = capacity;
    }

    void     swap( Vector<T>& other )
    {
      Swap( mTs, other.mTs );
      Swap( mTCount, other.mTCount );
      Swap( mTCapacity, other.mTCapacity );
    }

    dynmc T* begin() dynmc              { return mTs; };
    const T* begin() const              { return mTs; };
    dynmc T* end() dynmc                { return mTs + mTCount; };
    const T* end() const                { return mTs + mTCount; };
    dynmc T& front() dynmc              { TAC_ASSERT( mTCount ); return *mTs; }
    const T& front() const              { TAC_ASSERT( mTCount ); return *mTs; }
    dynmc T& back() dynmc               { TAC_ASSERT( mTCount ); return mTs[ mTCount - 1 ]; }
    const T& back() const               { TAC_ASSERT( mTCount ); return mTs[ mTCount - 1 ]; }
    dynmc T* data() dynmc               { return mTs; }
    const T* data() const               { return mTs; }

    // why do i use this fn?
    //T*       insert( T* pos, std::initializer_list< T > ts )
    //{
    //  const int iPos { int( pos - mTs ) };
    //  const int moveCount { mTCount - iPos };
    //  reserve( mTCount + ( int )ts.size() );
    //  pos = mTs + iPos;
    //  for( int i{}; i < moveCount; ++i )
    //    pos[ ts.size() + moveCount - i - 1 ] = Tac::move( pos[ moveCount - i - 1 ] );
    //  for( auto it : ts )
    //    *pos++ = it;
    //  mTCount += ( int )ts.size();
    //  return mTs + iPos;
    //}

    dynmc T& operator[]( int i ) dynmc  { TAC_ASSERT_INDEX( i, mTCount ); return mTs[ i ]; }
    const T& operator[]( int i ) const  { TAC_ASSERT_INDEX( i, mTCount ); return mTs[ i ]; }

    T*       mTs        {};
    int      mTCount    {};
    int      mTCapacity {};
  };

  template< typename T >
  bool operator == ( const Vector<T>& a, const Vector<T>& b )
  {
    const int n { a.size() };
    if( n != b.size() )
      return false;

    for( int i{}; i < n; ++i )
      if( a[ i ] != b[ i ] )
        return false;

    return true;
  }

  template< typename T >
  bool operator != ( const Vector<T>& a, const Vector<T>& b )
  {
    return !( a == b );
  }

} // namespace Tac
