#include "src/common/tacFrameMemory.h"
#include "src/common/containers/tacRingBuffer.h"

namespace Tac
{
  namespace FrameMemory
  {
    static thread_local RingBuffer* sFrameAllocator;
    void* Allocate( int byteCount )
    {
      return sFrameAllocator->Allocate( byteCount );
    }
    void SetThreadAllocator( RingBuffer* ringBuffer )
    {
      sFrameAllocator = ringBuffer;
    }
  }
}
