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
  template< typename T >
  struct Vector
  {
    Vector() = default;

    Vector( Vector&& v ) noexcept
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

    void operator =( Vector< T >&& v ) noexcept
    {
      const int n{ v.size() };
      resize( n );
      for( int i{}; i < n; ++i )
      {
        Swap( ( *this )[ i ], v[ i ] );
      }
      // 
    }

    void operator =( const Vector< T >& v )
    {
      assign( v.begin(), v.end() );
    }

    void assign( const T* begin, const T* end )
    {
      const int n { ( int )( end - begin ) };
      resize( n );
      for( int i{}; i < n; ++i )
        mTs[ i ] = begin[ i ];
    }

    void assign( int n, const T& t )
    {
      resize( n );
      for( int i{}; i < n; ++i )
        mTs[ i ] = t;
    }

    void clear()
    {
      for( int i{}; i < mTCount; ++i )
        mTs[ i ].~T();

      mTCount = 0;
    }

    void push_back( T&& t )
    {
      reserve();
      T* dst{ &mTs[ mTCount++ ] };
      TAC_PLACEMENT_NEW( dst )T( move( t ) );
    }

    void push_back( const T& t )
    {
      reserve();
      T* dst{ &mTs[ mTCount++ ] };
      TAC_PLACEMENT_NEW( dst )T( t );
    }

    template< class ... Args >
    auto emplace_back( Args&& ... args ) -> T&
    {
      reserve();
      TAC_PLACEMENT_NEW( &mTs[ mTCount++ ] )T( forward< Args>( args )... );
      return back();
    }

    void pop_back()
    {
      TAC_ASSERT( mTCount );
      mTs[ mTCount - 1 ].~T();
      mTCount--;
    }
    bool empty() const              { return !mTCount; }
    auto size() const -> int        { return mTCount; }

    void resize( const int newSize )
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

    void resize( int newSize, const T value )
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

    void reserve( int capacity )
    {
      if( capacity <= mTCapacity )
        return;

      T* newTs{ ( T* )Allocate( sizeof( T ) * capacity ) };
      for( int i{}; i < mTCount; ++i )
      {
        T& newT{ newTs[ i ] };
        T& oldT{ mTs[ i ] };
        TAC_PLACEMENT_NEW( &newT )T( Tac::move( oldT ) );
        oldT.~T();
      }

      Deallocate( mTs );
      mTs = newTs;
      mTCapacity = capacity;
    }

    void swap( Vector<T>& other )
    {
      Swap( mTs, other.mTs );
      Swap( mTCount, other.mTCount );
      Swap( mTCapacity, other.mTCapacity );
    }

    auto begin() dynmc -> dynmc T*              { return mTs; };
    auto begin() const -> const T*              { return mTs; };
    auto end() dynmc -> dynmc T*                { return mTs + mTCount; };
    auto end() const -> const T*                { return mTs + mTCount; };
    auto front() dynmc -> dynmc T&              { TAC_ASSERT( mTCount ); return *mTs; }
    auto front() const -> const T&              { TAC_ASSERT( mTCount ); return *mTs; }
    auto back() dynmc -> dynmc T&               { TAC_ASSERT( mTCount ); return mTs[ mTCount - 1 ]; }
    auto back() const -> const T&               { TAC_ASSERT( mTCount ); return mTs[ mTCount - 1 ]; }
    auto data() dynmc -> dynmc T*               { return mTs; }
    auto data() const -> const T*               { return mTs; }

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

    auto operator[]( int i ) dynmc -> dynmc T&  { TAC_ASSERT_INDEX( i, mTCount ); return mTs[ i ]; }
    auto operator[]( int i ) const -> const T&  { TAC_ASSERT_INDEX( i, mTCount ); return mTs[ i ]; }

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
