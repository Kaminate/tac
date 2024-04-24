#pragma once

namespace Tac
{
  // A Tac::RingBuffer is similar to a Tac::RingArray. Both are fixed sized buffers.
  // However, a Tac::RingBuffer is specialized for storing raw bytes,
  // and has a pop function that copies to a destination, accounting for wraparound.
  struct RingBuffer
  {
    void Init( int byteCount );
    bool Push( const void* dataBytes, int dataByteCount );
    bool Pop( void* dataBytes, int dataByteCount );
    bool Empty();
    int  size() const;
    int  capacity() const;

  private:
    int   mCapacity {};
    int   mIndex    {};
    int   mSize     {};
    char* mBytes    {};
  };

}

