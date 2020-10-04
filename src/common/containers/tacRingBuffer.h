#pragma once

namespace Tac
{
  struct RingBuffer
  {
    void Init( int byteCount );
    bool Push( const void* dataBytes, int dataByteCount );
    bool Pop( void* dataBytes, int dataByteCount );
    bool Empty();

  private:
    int   mCapacity = 0;
    int   mIndex = 0;
    int   mSize = 0;
    char* mBytes = nullptr;
  };
}

