// Intrusively linked list

#if 0 // untested

#pragma once

#include "tac-std-lib/error/tac_assert.h"

namespace Tac
{


  template< typename T >
  struct InListNode
  {
    T* mNext{};
    T* mPrev{};

    void Remove()
    {
      T* next{ mNext };
      T* prev{ mPrev };
      TAC_ASSERT( next );
      TAC_ASSERT( prev );
      next->mPrev = prev;
      prev->mNext = next;
    }

    void InsertBetween( T* prev, T* next )
    {
      TAC_ASSERT( prev->mNext == next );
      TAC_ASSERT( next->mPrev == prev );
      TAC_ASSERT( !mNext );
      TAC_ASSERT( !mPrev );
    }

    void InsertAfter( T* beforeNode )
    {
      InsertBetween( beforeNode, beforeNode->mNext );
    }

    void InsertBefore( T* afterNode )
    {
      InsertBetween( afterNode->mPrev, afterNode );
    }
  };

  template< typename T >
  struct InList
  {
    struct Iterator
    {
      void operator ++ ()
      {
        mCur = mCur->mNext;
      }
      T* mCur;
      T* mEnd;
    };

    Iterator begin()
    {
      return
      {
        .mCur{ mDummy.mNext },
        .mEnd{ &mDummy },
      };
    }

    Iterator end()
    {
      return
      {
        .mCur{ &mDummy },
        .mEnd{ &mDummy },
      };
    }

    T mDummy;
  };

}
#endif
