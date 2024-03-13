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

    Vector( const Vector& v )
    {
      const int size = v.size();
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

    Vector( const T* tbegin, const T* tend )
    {
      int size = ( int )( tend - tbegin );
      resize( size );
      for( int i = 0; i < size; ++i )
        mTs[ i ] = tbegin[ i ];
    }

    Vector( std::initializer_list< T > ts )    
    {
      assign( ts.begin(), ts.end() );
    }

    ~Vector() { TAC_DELETE[] mTs; }

    void     operator =( const Vector< T >& v )
    {
      assign( v.begin(), v.end() );
    }

    void     assign( const T* begin, const T* end )
    {
      const int newSize = ( int )( end - begin );
      reserve( newSize );
      for( int i = 0; i < newSize; ++i )
        mTs[ i ] = begin[ i ];
      mTCount = newSize;
    }

    void     assign( int n, const T& t )
    {
      reserve( n );
      for( int i = 0; i < n; ++i )
        mTs[ i ] = t;
      mTCount = n;
    }

    void     clear()
    {
      // Does not delete elements.
      // Use Vector<T>().swap(x) instead.
      mTCount = 0;
    }

    void     push_back( T t )
    {
      const int newSize = mTCount + 1;
      if( newSize > mTCapacity )
        reserve( int( newSize * 1.5f ) );
      mTs[ mTCount++ ] = t;
    }

    void     pop_back()                 { TAC_ASSERT( mTCount ); mTCount--; }
    bool     empty() const              { return !mTCount; }
    int      size() const               { return mTCount; }

    void     resize( int newSize, T newValues = T() )
    {
      reserve( newSize );

      for( int i = mTCount; i < newSize; ++i )
        mTs[ i ] = Tac::move( newValues );

      // Does this handle the case where newSize < mTCount?
      // ( destructing the old elements? )

      mTCount = newSize;
    }

    void     reserve( int capacity )
    {
      if( capacity <= mTCapacity )
        return;

      T* newTs = TAC_NEW T[ capacity ];
      for( int i = 0; i < mTCount; ++i )
        newTs[ i ] = Tac::move( mTs[ i ] );

      delete[] mTs;
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
      const int iPos = int( pos - mTs );
      const int moveCount = mTCount - iPos;
      reserve( mTCount + ( int )ts.size() );
      pos = mTs + iPos;
      for( int i = 0; i < moveCount; ++i )
        pos[ ts.size() + moveCount - i - 1 ] = Tac::move( pos[ moveCount - i - 1 ] ); // std::move( pos[ moveCount - i - 1 ] );
      for( auto it : ts )
        *pos++ = it;
      mTCount += ( int )ts.size();
      return mTs + iPos;
    }

    T&       operator[]( int i )        { TAC_ASSERT_INDEX( i, mTCount ); return mTs[ i ]; }
    const T& operator[]( int i ) const  { TAC_ASSERT_INDEX( i, mTCount ); return mTs[ i ]; }

    T*       mTs = nullptr;
    int      mTCount = 0;
    int      mTCapacity = 0;
  };

  template< typename T >
  bool operator == ( const Vector<T>& a, const Vector<T>& b )
  {
    const int n = a.size();
    if( n != b.size() )
      return false;

    for( int i = 0; i < n; ++i )
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
