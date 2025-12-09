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
  struct FifoQueue
  {
    FifoQueue() = default;
    FifoQueue( FifoQueue&& q ) noexcept { swap( ( FifoQueue&& )q ); }
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
      auto GetIndex()                { return ( mFifoQueue->mStartIndex + mOffset ) % mFifoQueue->mTCapacity; }
      auto operator *() -> const T&  { return mFifoQueue->mTs[ GetIndex() ]; }
      void operator ++()             { mOffset++; }
      bool operator ==( const ConstIterator& ) const = default;

      const FifoQueue* mFifoQueue    {};
      int              mOffset       {};
    };

    struct DynmcIterator
    {
      auto GetIndex()                { return ( mFifoQueue->mStartIndex + mOffset ) % mFifoQueue->mTCapacity; }
      auto operator *() -> dynmc T&  { return mFifoQueue->mTs[ GetIndex() ]; }
      void operator ++()             { mOffset++; }
      bool operator ==( const DynmcIterator& ) const = default;

      dynmc FifoQueue* mFifoQueue    {};
      int              mOffset       {};
    };

    void clear()
    {
      DynmcIterator i{ begin() };
      DynmcIterator e{ end() };
      for( ; i != e; ++i )
      {
        T& t = *i;
        t.~T();
      }

      mTCount = 0;
      mStartIndex = 0;
    }

    void push( T&& t )
    {
      reserve();
      mTCount++;
      TAC_PLACEMENT_NEW( &back() )T( move( t ) );
    }

    void push( const T& t )
    {
      reserve();
      mTCount++;
      TAC_PLACEMENT_NEW( &back() )T( t );
    }

    void reserve( int capacity )
    {
      if( capacity <= mTCapacity )
        return;

      auto newTs{ ( T* )Tac::Allocate( sizeof( T ) * capacity ) };
      for( int i{}; i < mTCount; ++i )
      {
        T& oldT{ mTs[ ( mStartIndex + i ) % mTCapacity ] };
        TAC_PLACEMENT_NEW( newTs + i )T( move( oldT ) );
      }

      Tac::Deallocate( mTs );
      mStartIndex = 0; // !
      mTCapacity = capacity;
      mTs = newTs;
    }

    void reserve()
    {
      if( mTCount == mTCapacity )
        reserve( int( mTCount * 1.5f ) + 1 );
    }

    template< class ... Args >
    auto emplace( Args&& ... args ) -> T&
    {
      reserve();
      mTCount++;
      TAC_PLACEMENT_NEW( &back() )T( forward< Args >( args )... );
      return back();
    }

    void pop()
    {
      TAC_ASSERT( mTCount );
      T& t{ front() };
      t.~T();
      mTCount--;
      mStartIndex = ( mStartIndex + 1 ) % mTCapacity;
    }
    bool empty() const              { return !mTCount; }
    auto size() const               { return mTCount; }

    void swap( FifoQueue<T>&& other ) noexcept
    {
      Swap( mTs, other.mTs );
      Swap( mTCount, other.mTCount );
      Swap( mTCapacity, other.mTCapacity );
      Swap( mStartIndex, other.mStartIndex );
    }

    auto begin() dynmc -> DynmcIterator        { return DynmcIterator{ .mFifoQueue{ this }, .mOffset{} }; }
    auto begin() const -> ConstIterator        { return ConstIterator{ .mFifoQueue{ this }, .mOffset{} }; }
    auto end() dynmc -> DynmcIterator          { return DynmcIterator{ .mFifoQueue{ this }, .mOffset{ mTCount } }; }
    auto end() const -> ConstIterator          { return ConstIterator{ .mFifoQueue{ this }, .mOffset{ mTCount } }; }

    // first element in the queue / the next element to be popped
    auto front() dynmc -> dynmc T&             { TAC_ASSERT( mTCount ); return mTs[ mStartIndex ]; }
    auto front() const -> const T&             { TAC_ASSERT( mTCount ); return mTs[ mStartIndex ]; }

    // last element in the queue / the most recently pushed element
    auto back() dynmc -> dynmc T&              { TAC_ASSERT( mTCount ); return mTs[ ( mStartIndex + mTCount - 1 ) % mTCapacity ]; }
    auto back() const -> const T&              { TAC_ASSERT( mTCount ); return mTs[ ( mStartIndex + mTCount - 1 ) % mTCapacity ]; }

    auto data() dynmc -> dynmc T*              { return mTs; }
    auto data() const -> const T*              { return mTs; }


    void operator =( FifoQueue< T >&& v )
    {
      swap( ( FifoQueue<T>&& )v );
    }

    void operator =( const FifoQueue< T >& v )
    {
      reserve( v.mTCount );
      for( T& t : v )
        push( t );
    }


  private:

    friend struct DynmcIterator;
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
