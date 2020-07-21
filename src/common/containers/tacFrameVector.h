#pragma once
#include <initializer_list>

#include "common/tacFrameMemory.h"
#include "common/tacPreprocessor.h"

namespace Tac
{
  namespace FrameMemory
  {
    template< typename T >
    struct Vector
    {
      Vector() = default;
      template< typename Iterator >
      Vector( Iterator iBegin, Iterator iEnd )
      {
        while( iBegin != iEnd )
          push_back( *iBegin++ );
      }
      Vector( const Vector& v )
      {
        int size = v.size();
        resize( size );
        for( int i = 0; i < size; ++i )
          mTs[ i ] = v[ i ];
      }
      Vector( int size )
      {
        resize( size );
      }
      Vector( int size, T initialValue )
      {
        resize( size );
        for( int i = 0; i < size; ++i )
          mTs[ i ] = initialValue;
      }
      Vector( T* tbegin, T* tend )
      {
        int size = ( int )( tend - tbegin );
        resize( size );
        for( int i = 0; i < size; ++i )
          mTs[ i ] = tbegin[ i ];
      }
      Vector( std::initializer_list< T > ts )
      {
        resize( ( int )ts.size() );
        int i = 0;
        for( T t : ts )
          mTs[ i++ ] = t;
      }
      void operator =( const Vector<T>& v )
      {
        int newSize = v.size();
        resize( newSize );
        for( int i = 0; i < newSize; ++i )
          mTs[ i ] = v[ i ];
      }
      void clear()
      {
        mTs = nullptr;
        mTCount = 0;
        mTCapacity = 0;
      }
      int size() const
      {
        return mTCount;
      }
      void push_back( T t )
      {
        const int newSize = mTCount + 1;
        if( newSize > mTCapacity )
          reserve( int( newSize * 1.5f ) );
        mTs[ mTCount++ ] = t;
      }
      void pop_back()
      {
        TAC_ASSERT( mTCount );
        mTCount--;
      }
      bool empty() const { return !mTCount; }
      void resize( int newSize )
      {
        if( newSize > mTCount )
          reserve( newSize );
        mTCount = newSize;
      }
      void reserve( int capacity )
      {
        if( capacity < mTCapacity )
          return;
        auto newTs = ( T* )Allocate( sizeof( T ) * capacity );
        for( int i = 0; i < mTCount; ++i )
          newTs[ i ] = std::move( mTs[ i ] );
        mTs = newTs;
        mTCapacity = capacity;
      }
      T* begin() { return mTs; };
      T* end() { return mTs + mTCount; };
      const T* begin() const { return mTs; };
      const T* end() const { return mTs + mTCount; };
      T& front() { return *begin(); }
      T& back() { return mTs[ mTCount - 1 ]; }
      T* data() { return mTs; }
      const T* data() const { return mTs; }
      T& operator[]( int i ) { return mTs[ i ]; }
      const T& operator[]( int i ) const { return mTs[ i ]; }
      T* mTs = nullptr;
      int mTCount = 0;
      int mTCapacity = 0;
    };
  }
}

