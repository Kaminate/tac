#pragma once
#include "src/common/tacMemory.h"
#include <initializer_list>  // std::initializer_list
#include <utility> // std::move

namespace Tac
{


  template< typename T >
  struct Vector
  {
    Vector() = default;
    //template< typename Iterator >
    //Vector( Iterator iBegin, Iterator iEnd )
    //{
    //  while( iBegin != iEnd )
    //  {
    //    T t = *iBegin++;
    //    push_back( t );
    //  }
    //}
    Vector( const Vector& v )
    {
      int size = v.size();
      resize( size );
      for( int i = 0; i < size; ++i )
        mTs[ i ] = v[ i ];
    }
    Vector( int size ) { resize( size ); }
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
    Vector( std::initializer_list< T > ts ) { assign( ts.begin(), ts.end() ); }
    //{
    //  resize( ( int )ts.size() );
    //  ts.size();
    //  ts.begin();
    //  int i = 0;
    //  for( T t : ts )
    //    mTs[ i++ ] = t;
    //}
    ~Vector() { delete[] mTs; }
    void     operator =( const Vector<T>& v )
    {
      assign( v.begin(), v.end() );
      //const int newSize = v.size();
      //resize( newSize );
      //for( int i = 0; i < newSize; ++i )
      //  mTs[ i ] = v[ i ];
    }
    void     assign( const T* begin, const T* end )
    {
      const int newSize = ( int )( end - begin );
      reserve( newSize );
      for( int i = 0; i < newSize; ++i )
        mTs[ i ] = begin[ i ];
      mTCount = newSize;
    }
    void     clear()
    {
      delete[] mTs;
      mTs = nullptr;
      mTCount = 0;
      mTCapacity = 0;
    }
    int      size() const
    {
      return mTCount;
    }
    void     push_back( T t )
    {
      const int newSize = mTCount + 1;
      if( newSize > mTCapacity )
        reserve( int( newSize * 1.5f ) );
      mTs[ mTCount++ ] = t;
    }
    void     pop_back()
    {
      TAC_ASSERT( mTCount );
      mTCount--;
    }
    bool     empty() const { return !mTCount; }
    void     resize( int newSize, T newValues = T() )
    {
      if( newSize > mTCount )
        reserve( newSize );
      for( int i = mTCount; i < newSize; ++i )
        mTs[ i ] = std::move( newValues );
      mTCount = newSize;
    }
    void     reserve( int capacity )
    {
      if( capacity <= mTCapacity )
        return;
      T* newTs = TAC_NEW T[ capacity ];
      for( int i = 0; i < mTCount; ++i )
        newTs[ i ] = std::move( mTs[ i ] );
      delete[] mTs;
      mTs = newTs;
      mTCapacity = capacity;
    }
    T*       begin() { return mTs; };
    const T* begin() const { return mTs; };
    T*       end() { return mTs + mTCount; };
    const T* end() const { return mTs + mTCount; };
    T&       front() { return *begin(); }
    T&       back() { return mTs[ mTCount - 1 ]; }
    T*       data() { return mTs; }
    const T* data() const { return mTs; }
    T&       operator[]( int i ) { return mTs[ i ]; }
    const T& operator[]( int i ) const { return mTs[ i ]; }
    T*       mTs = nullptr;
    int      mTCount = 0;
    int      mTCapacity = 0;
  };
}
