#include "src/common/tacFrameMemory.h"
#include "src/common/tacMemory.h"

#include <cstdarg> // va_list, va_start, va_end
#include <cstdio> // vsnprintf

namespace Tac
{

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

  static thread_local ThreadAllocator* sFrameAllocator;

  void* FrameMemoryAllocate( int byteCount )
  {
    return sFrameAllocator->Allocate( byteCount );
  }

  void  FrameMemorySetThreadAllocator( ThreadAllocator* threadAllocator )
  {
    sFrameAllocator = threadAllocator;
  }

  char* FrameMemoryPrintf( const char* format, ... )
  {
    va_list args;
    va_start( args, format );
    const int strlen = vsnprintf( nullptr, 0, format, args );
    if( strlen < 0 )
      return "";
    auto result = ( char* )FrameMemoryAllocate( strlen + 1 );
    vsnprintf( result, strlen, format, args );
    result[ strlen ] = '\0';
    va_end( args );
    return result;
  }
}
