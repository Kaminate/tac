#include "src/common/containers/tacRingBuffer.h"
#include "src/common/tacMemory.h"
#include "src/common/tacString.h"

namespace Tac
{
  int  RingBuffer::size() const
  {
    return mSize;
  }

  int  RingBuffer::capacity() const
  {
    return mCapacity;
  }

  bool RingBuffer::Push( const void* bytes, int byteCount )
  {
    TAC_ASSERT( mBytes );
    const int remainingByteCount = mCapacity - mSize;
    if( remainingByteCount < byteCount )
    {
      if( IsDebugMode() )
      {
        TAC_CRITICAL_ERROR_INVALID_CODE_PATH;
      }
      return false;
    }
    const int endIndex = ( mIndex + mSize ) % mCapacity;
    if( endIndex + byteCount > mCapacity )
    {
      char* src = ( char* )bytes;
      MemCpy( mBytes + endIndex,
              src,
              mCapacity - endIndex );
      MemCpy( mBytes,
              src + ( mCapacity - endIndex ),
              byteCount - ( mCapacity - endIndex ) );
    }
    else
    {
      MemCpy( mBytes + ( mIndex + mSize ) % mCapacity,
              bytes,
              byteCount );
    }
    mSize += byteCount;
    return true;
  }

  bool RingBuffer::Pop( void* dataBytes, int dataByteCount )
  {
    TAC_ASSERT( mBytes );
    if( mSize < dataByteCount )
    {
      if( IsDebugMode() )
      {
        TAC_CRITICAL_ERROR_INVALID_CODE_PATH;
      }
      return false;
    }

    const int origIndex = mIndex;
    const int remaining = mCapacity - mIndex;
    if( dataByteCount > remaining )
    {
      MemCpy( dataBytes, mBytes + mIndex, remaining );
      MemCpy( ( char* )dataBytes + remaining, mBytes, dataByteCount - remaining );
      mIndex = dataByteCount - remaining;
    }
    else
    {
      MemCpy( dataBytes, mBytes + mIndex, dataByteCount );
      mIndex = ( mIndex + dataByteCount ) % mCapacity;
    }

    if( IsDebugMode() )
      for( int i = 0; i < dataByteCount; ++i )
        mBytes[ ( origIndex + i ) % mCapacity ] = 0;

    mSize -= dataByteCount;
    return true;
  }

  bool RingBuffer::Empty()
  {
    return mSize == 0;
  }

  void RingBuffer::Init( int byteCount )
  {
    mBytes = TAC_NEW char[ byteCount ] {};
    mCapacity = byteCount;
  }

}

