#pragma once

namespace Tac
{
  struct RingBuffer
  {
    void  Init( int byteCount );
    void* Allocate( int byteCount );

  private:
    int   mCapacity = 0;
    int   mIndex = 0;
    char* mBytes = nullptr;
  };
}

