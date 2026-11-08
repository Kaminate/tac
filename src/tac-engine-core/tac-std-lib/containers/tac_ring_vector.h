#pragma once

#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/memory/tac_memory.h"

namespace Tac
{
  // Should be renamed to RingQueue
  // It is a FIFO (first in first out) structure
  template< typename T >
  struct RingVector
  {
    RingVector()                   { mTs = TAC_NEW T[ mCountAllocated = 10 ]; }
    ~RingVector()                  { TAC_DELETE[] mTs; }
    void Push( T t )
    {
      if( mCountUsed == mCountAllocated )
        Grow();

      mCountUsed++;
      mTs[ BackIdx() ] = t;
    }
    auto Pop() -> T
    {
      T t{ front() };
      mStartIndex = ( mStartIndex + 1 ) % mCountAllocated;
      mCountUsed--;
      return t;
    }
    auto front() dynmc -> dynmc T& { TAC_ASSERT( mCountUsed ); return mTs[ mStartIndex ]; }
    auto front() const -> const T& { TAC_ASSERT( mCountUsed ); return mTs[ mStartIndex ]; }
    auto back() dynmc -> dynmc T&  { return mTs[ BackIdx() ]; }
    auto back() const -> const T&  { return mTs[ BackIdx() ]; }
    auto size() const -> int       { return mCountUsed; }
    bool empty() const             { return mCountUsed == 0; }

  private:
    auto BackIdx() const -> int   { TAC_ASSERT( mCountUsed ); return ( mStartIndex + mCountUsed - 1 ) % mCountAllocated; }
    void Grow()
    {
        const int newCountAllocated { ( int )( mCountAllocated * 1.5f ) };
        T* newTs { TAC_NEW T[ newCountAllocated ] };

        for( int i{}; i < mCountUsed; ++i )
        {
          newTs[ i ] = mTs[ ( mStartIndex + i ) % mCountAllocated ];
        }

        TAC_DELETE[] mTs;
        mTs = newTs;
        mCountAllocated = newCountAllocated;
        mStartIndex = 0;
    }

    T*   mTs             {};
    int  mCountAllocated {};
    int  mStartIndex     {};
    int  mCountUsed      {};
  };
}

