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
      TAC_ASSERT_UNIMPLEMENTED;
    }

    FifoQueue( const FifoQueue& v )
    {
      TAC_ASSERT_UNIMPLEMENTED;
    }

    ~FifoQueue()
    {
      TAC_ASSERT_UNIMPLEMENTED;
    }

    void     operator =( FifoQueue< T >&& v )
    {
      TAC_ASSERT_UNIMPLEMENTED;
    }

    void     operator =( const FifoQueue< T >& v )
    {
      TAC_ASSERT_UNIMPLEMENTED;
    }

    void     clear()
    {
      TAC_ASSERT_UNIMPLEMENTED;
    }

    void     push( T&& t )
    {
      TAC_ASSERT_UNIMPLEMENTED;
    }

    void     push( const T& t )
    {
      TAC_ASSERT_UNIMPLEMENTED;
    }

    template< class ... Args >
    T&       emplace( Args&& ... args )
    {
      TAC_ASSERT_UNIMPLEMENTED;
      return back();
    }

    void     pop()                
    {
      TAC_ASSERT_UNIMPLEMENTED;
    }
    bool     empty() const              { return !mTCount; }
    int      size() const               { return mTCount; }

    void     swap( FifoQueue<T>& other )
    {
      TAC_ASSERT_UNIMPLEMENTED;
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
    TAC_ASSERT_UNIMPLEMENTED;
    return {};
  }

  template< typename T >
  bool operator != ( const FifoQueue<T>& a, const FifoQueue<T>& b )
  {
    TAC_ASSERT_UNIMPLEMENTED;
    return {};
  }

} // namespace Tac
