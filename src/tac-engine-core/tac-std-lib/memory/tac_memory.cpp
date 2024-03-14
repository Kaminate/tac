#include "tac_memory.h" // self-inc

//#include "tac-desktop-app/tac_desktop_app.h" // temp
//#include "tac-std-lib/shell/tac_shell.h"
//#include "tac-engine-core/shell/tac_shell_timestep.h"

namespace Tac
{
  static thread_local StackFrame sNewStackFrame;

  // these should be atomic
  static int memAllocCounter;
  static int memFreeCounter;
  static int memCount;

  static void* Allocate( const StackFrame& stackFrame, const std::size_t sz )
  {
    TAC_UNUSED_PARAMETER( stackFrame );
    void* result = std::malloc( sz );

    // track dynamic memory allocations
    //if( Timestep::GetElapsedTime() > 2)
    {
      ++memAllocCounter;
      ++memCount;
    }
    return result;
  }

  static void  Deallocate( void* ptr )
  {
    {
      ++memFreeCounter;
      --memCount;
    }
    std::free( ptr );
  }
}

void Tac::SetNewStackFrame( const StackFrame& stackFrame )
{
  sNewStackFrame = stackFrame;
}

void  operator delete( void* ptr ) noexcept
{
  Tac::Deallocate( ptr );
}

void operator delete( void* ptr, const std::size_t ) noexcept
{
  Tac::Deallocate( ptr );
}

void  operator delete( void* ptr, const Tac::Happy ) noexcept
{
  Tac::Deallocate( ptr );
}

void* operator new( const std::size_t sz )
{
  return Tac::Allocate( Tac::sNewStackFrame, sz );
}

void* operator new( std::size_t sz, Tac::Happy )
{
  return Tac::Allocate( Tac::sNewStackFrame, sz );
}
