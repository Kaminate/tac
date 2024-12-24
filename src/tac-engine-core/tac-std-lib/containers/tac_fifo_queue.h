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


    // Note: mOffset prevents begin() == end() when count == capacity
    struct ConstIterator
    {
      int        GetIndex()    { return ( mFifoQueue->mStartIndex + mOffset ) % mFifoQueue->mTCapacity; }

      const T&   operator *()  { return mFifoQueue->mTs[ GetIndex() ]; }
      void       operator ++() { mOffset++; }
      bool       operator ==( const ConstIterator& ) const = default;

      FifoQueue* mFifoQueue    {};
      int        mOffset       {};
    };

    struct Iterator
    {
      int        GetIndex()    { return ( mFifoQueue->mStartIndex + mOffset ) % mFifoQueue->mTCapacity; }

      dynmc T&   operator *()  { return mFifoQueue->mTs[ GetIndex() ]; }
      void       operator ++() { mOffset++; }
      bool       operator ==( const Iterator& ) const = default;

      FifoQueue* mFifoQueue    {};
      int        mOffset       {};
    };

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
      Iterator i = begin();
      Iterator e = end();
      for( ; i != e; ++i )
      {
        T& t = *i;
        t.~T();
      }
      //for( T& t : *this )
      //  t.~T();

      mTCount = 0;
      mStartIndex = 0;
    }

    void     push( T&& t )
    {
      reserve();
      mTCount++;
      TAC_PLACEMENT_NEW( &back() )T( move( t ) );
    }

    void     push( const T& t )
    {
      reserve();
      mTCount++;
      TAC_PLACEMENT_NEW( &back() )T( t );
    }

    void reserve( int capacity )
    {
      if( capacity <= mTCapacity )
        return;

      T* newTs{ ( T* )Tac::Allocate( sizeof( T ) * capacity ) };
      T* newT{ newTs };
      for( T& t : *this )
      {
        TAC_PLACEMENT_NEW( newT++ )T( move( t ) );
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
      mTCount++;
      TAC_PLACEMENT_NEW( &back() )T( forward< Args >( args )... );
      return back();
    }

    void     pop()                
    {
      TAC_ASSERT( mTCount );
      T& t{ front() };
      t.~T();
      mTCount--;
      mStartIndex = ( mStartIndex + 1 ) % mTCapacity;
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

    Iterator      begin() dynmc         { return Iterator     { .mFifoQueue{ this }, .mOffset{} }; }
    ConstIterator begin() const         { return ConstIterator{ .mFifoQueue{ this }, .mOffset{} }; }
    Iterator      end() dynmc           { return Iterator     { .mFifoQueue{ this }, .mOffset{ mTCount } }; }
    ConstIterator end() const           { return ConstIterator{ .mFifoQueue{ this }, .mOffset{ mTCount } }; }

    // first element in the queue / the next element to be popped
    dynmc T& front() dynmc              { TAC_ASSERT( mTCount ); return mTs[ mStartIndex ]; }
    const T& front() const              { TAC_ASSERT( mTCount ); return mTs[ mStartIndex ]; }

    // last element in the queue / the most recently pushed element
    dynmc T& back() dynmc               { TAC_ASSERT( mTCount ); return mTs[ ( mStartIndex + mTCount - 1 ) % mTCapacity ]; }
    const T& back() const               { TAC_ASSERT( mTCount ); return mTs[ ( mStartIndex + mTCount - 1 ) % mTCapacity ]; }

    dynmc T* data() dynmc               { return mTs; }
    const T* data() const               { return mTs; }

  private:

    friend struct Iterator;
    friend struct ConstIterator;

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
