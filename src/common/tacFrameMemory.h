#pragma once

namespace Tac
{
  struct RingBuffer;
  namespace FrameMemory
  {
    void SetThreadAllocator( RingBuffer* );
    void* Allocate( int byteCount );
  }
}

