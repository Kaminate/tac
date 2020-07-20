#include "src/common/containers/tacRingBuffer.h"
#include "src/common/tacMemory.h"

namespace Tac
{

  void RingBuffer::Init( int byteCount )
  {
    mBytes = TAC_NEW char[ byteCount ];
    mCapacity = byteCount;
  }
  void* RingBuffer::Allocate( int byteCount )
  {
    if( mIndex + byteCount > mCapacity )
      mIndex = 0;
    void* result = mBytes + mIndex;
    mIndex += byteCount;
    return result;
  }
}

