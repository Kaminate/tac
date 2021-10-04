#include "src/common/tacFrameMemory.h"
#include "src/common/tacMemory.h"
#include "src/common/containers/tacArray.h"

#include <cstdarg> // va_list, va_start, va_end
#include <cstdio> // vsnprintf

namespace Tac
{

  ThreadAllocator::ThreadAllocator( char* bytes, int byteCount )
  {
    mBytes = bytes;
    mCapacity = byteCount;
  }

  void ThreadAllocator::Init( int byteCount )
  {
    mBytes = TAC_NEW char[ byteCount ];
    mCapacity = byteCount;
  }

  void* ThreadAllocator::Allocate( int byteCount )
  {
    if( mIndex + byteCount > mCapacity )
      mIndex = 0;
    void* result = mBytes + mIndex;
    mIndex += byteCount;
    return result;
  }

  static Array< char, 100 > sTempBuf;
  static ThreadAllocator sInitialAllocator( sTempBuf.data(), sTempBuf.size() );
  static thread_local ThreadAllocator* sFrameAllocator = &sInitialAllocator;

  void* FrameMemoryAllocate( int byteCount )
  {
    return sFrameAllocator->Allocate( byteCount );
  }

  void  FrameMemorySetThreadAllocator( ThreadAllocator* threadAllocator )
  {
    sFrameAllocator = threadAllocator;
  }

  const char* FrameMemoryPrintf( const char* format, ... )
  {
    va_list args;
    va_start( args, format );
    const int strlen = vsnprintf( nullptr, 0, format, args );
    if( strlen < 0 )
      return "";
    const int buflen = strlen + 1;
    auto result = ( char* )FrameMemoryAllocate( buflen );
    vsnprintf( result, buflen, format, args );
    result[ strlen ] = '\0';
    va_end( args );
    return result;
  }
}
