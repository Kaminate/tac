#pragma once

#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/memory/tac_memory.h"

namespace Tac
{

  template< typename T, int N >
  struct RingArray
  {
    RingArray()           { mTs = TAC_NEW T[ N ]; }
    ~RingArray()          { TAC_DELETE[] mTs; }
    int  size() const     { return mSize; }
    int  capacity() const { return N; }
    T&   back()           { return mTs[ ( mIndex + mSize - 1 + N ) % N ]; }
    T&   front()          { return mTs[ mIndex ]; }
    void clear()          { mSize = 0; mIndex = 0; }
    void push_back( const T& );
    void push_front( const T& );
    T    pop_front();
    T    pop_back();
    T&   operator[]( int );

  private:

    void DecrementIndex() { mIndex = ( mIndex - 1 + N ) % N; }
    void IncrementIndex() { mIndex = ( mIndex + 1 ) % N; }

    T*   mTs;
    int  mSize = 0;
    int  mIndex = 0;
  };

  template< typename T, int N >
  void RingArray<T,N>::push_back( const T& t )
  {
    TAC_ASSERT( mSize < N );
    mSize++;
    back() = t;
  }

  template< typename T, int N >
  void RingArray<T,N>::push_front( const T& t )
  {
    TAC_ASSERT( mSize < N );
    DecrementIndex();
    front() = t;
    mSize++;
  }

  template< typename T, int N >
  T    RingArray<T,N>::pop_front()
  {
    TAC_ASSERT( mSize );
    T t = front();
    IncrementIndex();
    mSize--;
    return t;
  }

  template< typename T, int N >
  T    RingArray<T,N>::pop_back()
  {
    TAC_ASSERT( mSize );
    T t = back();
    mSize--;
    return t;
  }

  template< typename T, int N >
  T&   RingArray<T,N>::operator[]( int i )
  {
    // cool or evil?
    return mTs[ ( mIndex + i + N ) % N ];
  }


 } // namespace Tac

