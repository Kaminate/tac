#include "src/common/tacFrameMemory.h"
#include "src/common/tacMemory.h"


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

  namespace FrameMemory
  {


    static thread_local ThreadAllocator* sFrameAllocator;
    void* Allocate( int byteCount )
    {
      return sFrameAllocator->Allocate( byteCount );
    }

    void SetThreadAllocator( ThreadAllocator* threadAllocator )
    {
      sFrameAllocator = threadAllocator;
    }
  }
}
