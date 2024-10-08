#pragma once

#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/algorithm/tac_algorithm.h" // Swap

import std; // std::initializer_list
//#include <initializer_list>  // std::initializer_list
//#include <utility> // std::move

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
      {
        mTs[ i ].~T();
      }

      mTCount = 0;
    }

    void     push_back( T&& t )
    {
      if( mTCount == mTCapacity )
        reserve( int( mTCount * 1.5f ) + 1 );

      mTs[ mTCount++ ] = move( t );
    }

    void     push_back( const T& t )
    {
      if( mTCount == mTCapacity )
        reserve( int( mTCount * 1.5f ) + 1 );

      new( &mTs[ mTCount++ ] )T( t );
    }

    void     pop_back()                 { TAC_ASSERT( mTCount ); mTCount--; }
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
          new( &mTs[ i ] )T();
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

    void     reserve( int capacity )
    {
      if( capacity <= mTCapacity )
        return;

      T* newTs{ ( T* )Allocate( sizeof( T ) * capacity ) };
      for( int i{}; i < mTCount; ++i )
        newTs[ i ] = Tac::move( mTs[ i ] );

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

    T*       begin()                    { return mTs; };
    const T* begin() const              { return mTs; };
    T*       end()                      { return mTs + mTCount; };
    const T* end() const                { return mTs + mTCount; };
    T&       front()                    { TAC_ASSERT( mTCount ); return *mTs; }
    const T& front() const              { TAC_ASSERT( mTCount ); return *mTs; }
    T&       back()                     { TAC_ASSERT( mTCount ); return mTs[ mTCount - 1 ]; }
    const T& back() const               { TAC_ASSERT( mTCount ); return mTs[ mTCount - 1 ]; }
    T*       data()                     { return mTs; }
    const T* data() const               { return mTs; }

    // why do i use this fn?
    T*       insert( T* pos, std::initializer_list< T > ts )
    {
      const int iPos { int( pos - mTs ) };
      const int moveCount { mTCount - iPos };
      reserve( mTCount + ( int )ts.size() );
      pos = mTs + iPos;
      for( int i{}; i < moveCount; ++i )
        pos[ ts.size() + moveCount - i - 1 ] = Tac::move( pos[ moveCount - i - 1 ] ); // std::move( pos[ moveCount - i - 1 ] );
      for( auto it : ts )
        *pos++ = it;
      mTCount += ( int )ts.size();
      return mTs + iPos;
    }

    T&       operator[]( int i )        { TAC_ASSERT_INDEX( i, mTCount ); return mTs[ i ]; }
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
