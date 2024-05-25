// Intrusively linked list

#pragma once

#include "tac-std-lib/error/tac_assert.h"

namespace Tac
{


  template< typename T >
  struct InListNode
  {
    InListNode* mNext{};
    InListNode* mPrev{};

    void Remove()
    {
      InListNode* next{ mNext };
      InListNode* prev{ mPrev };
      TAC_ASSERT( next );
      TAC_ASSERT( prev );
      next->mPrev = prev;
      prev->mNext = next;
    }

    void InsertBetween( InListNode* prev, InListNode* next )
    {
      TAC_ASSERT( prev->mNext == next );
      TAC_ASSERT( next->mPrev == prev );
      TAC_ASSERT( !mNext );
      TAC_ASSERT( !mPrev );
    }

    void InsertAfter( InListNode* beforeNode )
    {
      InsertBetween( beforeNode, beforeNode->mNext );
    }

    void InsertBefore( InListNode* afterNode )
    {
      InsertBetween( afterNode->mPrev, afterNode );
    }
  };


}
