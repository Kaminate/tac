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
    RingVector()           { mTs = TAC_NEW T[ mCountAllocated = 10 ]; }
    ~RingVector()          { TAC_DELETE[] mTs; }
    void     Push( T t )
    {
      if( mCountUsed == mCountAllocated )
        Grow();

      mTs[ LastIdx() ] = t;
      mCountUsed++;
    }
    T        Pop()
    {
      TAC_ASSERT( mCountUsed );
      T t { mTs[ mStartIndex ] };
      mStartIndex = ( mStartIndex + 1 ) % mCountAllocated;
      mCountUsed--;
      return t;
    }
    T&       front()       { TAC_ASSERT( mCountUsed ); return mTs[ mStartIndex ]; }
    const T& front() const { TAC_ASSERT( mCountUsed ); return mTs[ mStartIndex ]; }
    T&       back()        { TAC_ASSERT( mCountUsed ); return mTs[ LastIdx() ]; }
    const T& back() const  { TAC_ASSERT( mCountUsed ); return mTs[ LastIdx() ]; }
    int      size() const  { return mCountUsed; }
    bool     empty() const { return mCountUsed == 0; }

  private:
    int  LastIdx() const   { return ( mStartIndex + mCountUsed ) % mCountAllocated; }
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
    T* mTs               {};
    int  mCountAllocated {};
    int  mStartIndex     {};
    int  mCountUsed      {};
  };
}

