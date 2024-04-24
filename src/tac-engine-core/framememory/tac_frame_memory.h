#pragma once

namespace Tac
{
  struct StringView;

  struct ThreadAllocator
  {
    ThreadAllocator() = default;
    ThreadAllocator( char*, int );
    void  Init( int byteCount );
    void* Allocate( int byteCount );
    bool  IsAllocated( const void* );

  private:
    int   mCapacity {};
    int   mIndex    {};
    char* mBytes    {};
  };


  void        FrameMemoryInitThreadAllocator( int byteCount );
  void        FrameMemorySetThreadAllocator( ThreadAllocator* );
  void*       FrameMemoryAllocate( int );
  StringView  FrameMemoryCopy( const StringView& );
  StringView  FrameMemoryCopy( const void*, int n);
  bool        FrameMemoryIsAllocated( const void* );


} // namespace Tac

