#pragma once

#include "src/common/tacPreprocessor.h"

namespace Tac
{
  template< typename T >
  struct RingVector
  {
    RingVector()
    {
      mTs = new T[ mCountAllocated = 10 ];
    }
    ~RingVector()
    {
      delete[] mTs;
    }
    void Push( T t )
    {
      if( mCountUsed == mCountAllocated )
      {
        auto newCountAllocated = ( int )( mCountAllocated * 1.5f );
        T* newTs = new T[ newCountAllocated ];

        for( int i = 0; i < mCountUsed; ++i )
        {
          newTs[ i ] = mTs[ ( mStartIndex + i ) % mCountAllocated ];
        }

        delete mTs;
        mTs = newTs;
        mCountAllocated = newCountAllocated;
        mStartIndex = 0;
      }

      mTs[ ( mStartIndex + mCountUsed ) % mCountAllocated ] = t;
      mCountUsed++;
    }
    T    Pop()
    {
      TAC_ASSERT( mCountUsed );
      T t = mTs[ mStartIndex ];
      mStartIndex = ( mStartIndex + 1 ) % mCountAllocated;
      mCountUsed--;
      return t;
    }
    int  size() { return mCountUsed; }
    bool empty() { return mCountUsed == 0; }
    T*   mTs;
    int  mCountAllocated;
    int  mStartIndex = 0;
    int  mCountUsed = 0;
  };
}

