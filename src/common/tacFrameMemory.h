#pragma once

namespace Tac
{
  struct ThreadAllocator
  {
    ThreadAllocator() = default;
    ThreadAllocator(char*, int);
    void  Init( int byteCount );
    void* Allocate( int byteCount );

  private:
    int   mCapacity = 0;
    int   mIndex = 0;
    char* mBytes = nullptr;
  };


  void  FrameMemorySetThreadAllocator( ThreadAllocator* );
  void* FrameMemoryAllocate( int );
  const char* FrameMemoryPrintf( const char*, ... );
}

