#include "src/common/tacPreprocessor.h"
#include "src/common/tacUtility.h"
#include "src/common/tacOS.h"
#include <cstdarg>

namespace Tac
{
  char* va( const char* format, ... )
  {
    const int bufferCount = 512;
    static thread_local char buffer[ bufferCount ];
    va_list args;
    va_start( args, format );
    vsnprintf( buffer, bufferCount, format, args );
    va_end( args );
    return buffer;
  }

  bool IsDebugMode()
  {
#ifdef NDEBUG
    return false;
#else
    return true;
#endif
  }

  StackFrame::StackFrame( const int line,
                          const char* file,
                          const char* function )
  {
    mLine = line;
    mFile = file;
    mFunction = function;
  }

  const char* StackFrame::ToString()  const
  {
    SplitFilepath splitFilepath( mFile );
    return va( "%s:%i %s", splitFilepath.mFilename.c_str(), mLine, mFunction );
  }

  void AssertInternal( const char* message, const StackFrame& frame )
  {
    OS::DebugAssert( message, frame );
  }
}
