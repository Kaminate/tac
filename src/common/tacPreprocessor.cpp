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

  StackFrame::StackFrame( int line, StringView file, StringView function )
  {
    mLine = line;
    mFile = file;
    mFunction = function;
  }

  String StackFrame::ToString()  const
  {
    SplitFilepath splitFilepath( mFile );
    String pathString = splitFilepath.mFilename;
    String lineString = Tac::ToString( mLine );
    String result = pathString + ":" + lineString + " " + mFunction;
    return result;
  }

  void AssertInternal( const String& message, const StackFrame& frame )
  {
    OS::DebugAssert( message, frame );
  }
}
