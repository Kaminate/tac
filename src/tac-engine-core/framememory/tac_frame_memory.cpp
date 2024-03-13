#include "tac-std-lib/memory/tac_frame_memory.h" // self-inc

#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/string/tac_string.h"

namespace Tac
{
  struct ThreadAllocator
  {
    ThreadAllocator() = default;
    ThreadAllocator(char*, int);
    void  Init( int byteCount );
    void* Allocate( int byteCount );
    bool  IsAllocated( const void* );

  private:
    int   mCapacity = 0;
    int   mIndex = 0;
    char* mBytes = nullptr;
  };

  // -----------------------------------------------------------------------------------------------

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

  bool  ThreadAllocator::IsAllocated( const void* ptr )
  {
    return ptr >= mBytes && ptr < mBytes + mCapacity;
  }

  // -----------------------------------------------------------------------------------------------

  static Array< char, 100 >            sTmpBuf;
  static ThreadAllocator               sTmpAllocator( sTmpBuf.data(), sTmpBuf.size() );
  static thread_local ThreadAllocator* sFrameAllocator = &sTmpAllocator;
  static thread_local ThreadAllocator  sFrameAllocatorInstance;

  // -----------------------------------------------------------------------------------------------

  void*       FrameMemoryAllocate( int byteCount )
  {
    return sFrameAllocator->Allocate( byteCount );
  }

  void        FrameMemoryInitThreadAllocator( int byteCount )
  {
    TAC_ASSERT( sFrameAllocator == &sTmpAllocator );
    sFrameAllocator = &sFrameAllocatorInstance;
    sFrameAllocator->Init( byteCount );
  }

  StringView  FrameMemoryCopy( const void* src, int n )
  {
    auto dst = ( char* )FrameMemoryAllocate( n );
    MemCpy( dst, src, n );
    return StringView( dst, n );
  }

  StringView  FrameMemoryCopy( const StringView& s)
  {
    const char* src = s.data();
    const int n = s.size();
    auto dst = ( char* )FrameMemoryAllocate( n + 1 );
    MemCpy( dst, src, n );
    dst[ n ] = '\0';
    return StringView( dst, n );
  }

  bool        FrameMemoryIsAllocated( const void* ptr )
  {
    return sFrameAllocator->IsAllocated( ptr );
  }

} // namespace Tac
