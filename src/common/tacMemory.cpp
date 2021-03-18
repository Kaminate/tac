#include "src/common/tacMemory.h"
#include "src/shell/tacDesktopApp.h" // temp
#include "src/common/shell/tacShell.h"
#include "src/common/shell/tacShellTimer.h"

#include <fstream>

namespace Tac
{
  static thread_local StackFrame sNewStackFrame;
  void SetNewStackFrame( StackFrame stackFrame )
  {
    sNewStackFrame = stackFrame;
  }
  static void* Allocate( StackFrame stackFrame, std::size_t sz )
  {
    TAC_UNUSED_PARAMETER( stackFrame );
    void* result = std::malloc( sz );

    // track dynamic memory allocations
    if( ShellGetElapsedSeconds() > 2)
    {
      static int memAllocCounter;
      ++memAllocCounter;
    }
    return result;
  }
  static void Deallocate( void* ptr )
  {
    std::free( ptr );
  }
}

void operator delete( void* ptr ) noexcept
{
  Tac::Deallocate( ptr );
}

void operator delete( void* ptr, Tac::Happy ) noexcept
{
  Tac::Deallocate( ptr );
}

void* operator new( std::size_t sz )
{
  return Tac::Allocate( Tac::sNewStackFrame, sz );
}

void* operator new( std::size_t sz, Tac::Happy )
{
  return Tac::Allocate( Tac::sNewStackFrame, sz );
}
