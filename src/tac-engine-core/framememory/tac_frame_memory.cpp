#include "tac_frame_memory.h" // self-inc

#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/error/tac_assert.h"

namespace Tac
{
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
    TAC_ASSERT( byteCount < mCapacity );
    if( mIndex + byteCount > mCapacity )
      mIndex = 0;
    void* result { mBytes + mIndex };
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
}

void        Tac::FrameMemorySetThreadAllocator( ThreadAllocator* allocator )
{
  sFrameAllocator = allocator;
}

void*       Tac::FrameMemoryAllocate( int byteCount )
{
  return sFrameAllocator->Allocate( byteCount );
}

void        Tac::FrameMemoryInitThreadAllocator( int byteCount )
{
  TAC_ASSERT( sFrameAllocator == &sTmpAllocator );
  sFrameAllocator = &sFrameAllocatorInstance;
  sFrameAllocator->Init( byteCount );
}

Tac::StringView  Tac::FrameMemoryCopy( const void* src, int n )
{
  auto dst { ( char* )FrameMemoryAllocate( n ) };
  MemCpy( dst, src, n );
  return StringView( dst, n );
}

Tac::StringView  Tac::FrameMemoryCopy( const StringView& s)
{
  const char* src { s.data() };
  const int n { s.size() };
  auto dst { ( char* )FrameMemoryAllocate( n + 1 ) };
  MemCpy( dst, src, n );
  dst[ n ] = '\0';
  return StringView( dst, n );
}

bool        Tac::FrameMemoryIsAllocated( const void* ptr )
{
  return sFrameAllocator->IsAllocated( ptr );
}

