#pragma once

namespace Tac
{

  struct ThreadAllocator
  {
    void  Init( int byteCount );
    void* Allocate( int byteCount );

  private:
    int   mCapacity = 0;
    int   mIndex = 0;
    char* mBytes = nullptr;
  };


  namespace FrameMemory
  {
    void SetThreadAllocator( ThreadAllocator* );
    void* Allocate( int );
  }
}

