#include "tac_memory.h" // self-inc

//#include "tac-desktop-app/desktop_app/tac_desktop_app.h" // temp
//#include "tac-engine-core/shell/tac_shell.h"
//#include "tac-engine-core/shell/tac_shell_timestep.h"

#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <cstdlib>
  #include <cstring>
#endif

namespace Tac
{
  static thread_local StackFrame sNewStackFrame;

  // these should be atomic
  static int memAllocCounter;
  static int memFreeCounter;
  static int memCount;
}

// Generally speaking, there should be no dynmaic allocations per frame
void* Tac::Allocate( const std::size_t sz )
{
  void* result{ std::malloc( sz ) };
  
  if constexpr( kIsDebugMode )
  {
    // hmm
    std::memset( result, 0, sz );
  }

  // track dynamic memory allocations
  //if( Timestep::GetElapsedTime() > 2)
  {
    ++memAllocCounter;
    ++memCount;
  }
  return result;
}

void* Tac::Allocate( const std::size_t sz, const StackFrame stackFrame )
{
  TAC_UNUSED_PARAMETER( stackFrame );
  return Tac::Allocate( sz );
}

void  Tac::Deallocate( void* ptr )
{
  if( ptr )
  {
    ++memFreeCounter;
    --memCount;
    std::free( ptr );
  }
}

void  Tac::SetNewStackFrame( const StackFrame& stackFrame )
{
  sNewStackFrame = stackFrame;
}

void  operator delete( void* ptr ) noexcept
{
  Tac::Deallocate( ptr );
}

void  operator delete( void* ptr, const std::size_t ) noexcept
{
  Tac::Deallocate( ptr );
}

void  operator delete( void* ptr, const Tac::Happy ) noexcept
{
  Tac::Deallocate( ptr );
}

void* operator new( const std::size_t sz )
{
  return Tac::Allocate( sz, Tac::sNewStackFrame );
}

void* operator new( const std::size_t sz, Tac::Happy )
{
  return Tac::Allocate( sz, Tac::sNewStackFrame );
}
