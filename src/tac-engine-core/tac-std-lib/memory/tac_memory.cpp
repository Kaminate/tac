#include "tac_memory.h" // self-inc

//#include "tac-desktop-app/desktop_app/tac_desktop_app.h" // temp
//#include "tac-engine-core/shell/tac_shell.h"
//#include "tac-engine-core/shell/tac_shell_timestep.h"

namespace Tac
{
  static thread_local StackFrame sNewStackFrame;

  // these should be atomic
  static int memAllocCounter;
  static int memFreeCounter;
  static int memCount;
}

  void* Tac::Allocate( const std::size_t sz )
  {
    void* result { std::malloc( sz ) };
    std::memset( result, 0, sz );

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
    void* result { std::malloc( sz ) };
    std::memset( result, 0, sz );

    // track dynamic memory allocations
    //if( Timestep::GetElapsedTime() > 2)
    {
      ++memAllocCounter;
      ++memCount;
    }
    return result;
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
  return Tac::Allocate( sz, Tac::sNewStackFrame );
}

void* operator new( std::size_t sz, Tac::Happy )
{
  return Tac::Allocate( sz, Tac::sNewStackFrame );
}
