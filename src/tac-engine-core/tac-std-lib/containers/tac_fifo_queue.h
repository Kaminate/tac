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
  struct FifoQueue
  {
    FifoQueue() = default;

    FifoQueue( FifoQueue&& q )
    {
      swap( q );
    }

    FifoQueue( const FifoQueue& v )
    {
      reserve( v.mTCount );
      for( T& t : v )
        push( t );
    }

    ~FifoQueue()
    {
      clear();
      Deallocate( mTs );
      mTs = {};
      mTCapacity = {};
    }

    void     operator =( FifoQueue< T >&& v )
    {
      swap( v );
    }

    void     operator =( const FifoQueue< T >& v )
    {
      reserve( v.mTCount );
      for( T& t : v )
        push( t );
    }

    void     clear()
    {
      for( int i{ mStartIndex };
           i != ( mStartIndex + mTCount );
           i = ( i + 1 ) % mTCapacity )
        mTs[ i ].~T();

      mTCount = 0;
      mStartIndex = 0;
    }

    void     push( T&& t )
    {
      reserve();
      T* dst{ &mTs[ ( mStartIndex + mTCount++ ) % mTCapacity ] };
      TAC_NEW( dst )T( move( t ) ); // placement new using T(T&&)
    }

    void     push( const T& t )
    {
      reserve();
      T* dst{ &mTs[ ( mStartIndex + mTCount++ ) % mTCapacity ] };
      TAC_NEW( dst )T( t ); // placement new using T(const T&)
    }

    void reserve( int capacity )
    {
      if( capacity <= mTCapacity )
        return;

      T* newTs{ ( T* )Tac::Allocate( sizeof( T ) * capacity ) };
      for( int iDst{}; iDst < mTCount; ++iDst )
      {
        const int iSrc { ( mStartIndex + iDst ) % mTCapacity };
        TAC_NEW ( &newTs[ iDst ] )T ( move( mTs[ iSrc ] ) );
      }

      Tac::Deallocate( mTs );
      mTs = newTs;
      mTCapacity = capacity;
    }

    void     reserve()
    {
      if( mTCount == mTCapacity )
        reserve( int( mTCount * 1.5f ) + 1 );
    }

    template< class ... Args >
    T&       emplace( Args&& ... args )
    {
      reserve();
      TAC_NEW( &mTs[ mTCount++ ] )T( forward< Args >( args )... );
      return back();
    }

    void     pop()                
    {
      TAC_ASSERT( mTCount );
      mTs[ mStartIndex + mTCount - 1 ].~T();
      mTCount--;
    }
    bool     empty() const              { return !mTCount; }
    int      size() const               { return mTCount; }

    void     swap( FifoQueue<T>& other )
    {
      Swap( mTs, other.mTs );
      Swap( mTCount, other.mTCount );
      Swap( mTCapacity, other.mTCapacity );
      Swap( mStartIndex, other.mStartIndex );
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

    T*       mTs         {};
    int      mTCount     {};
    int      mTCapacity  {};
    int      mStartIndex {};
  };

  template< typename T >
  bool operator == ( const FifoQueue<T>& a, const FifoQueue<T>& b )
  {
    if( a.size() != b.size() ) { return false; }
    const int n{ a.size() };
    for( int i{}; i < n; ++i )
      if( a[i] != b[i] )
        return false;
    return true;
  }

  template< typename T >
  bool operator != ( const FifoQueue<T>& a, const FifoQueue<T>& b )
  {
    return !( a == b );
  }

  void FifoQueueUnitTest();

} // namespace Tac
