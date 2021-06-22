#include "src/common/tacPreprocessor.h"
#include "src/common/tacUtility.h"
#include "src/common/tacOS.h"
#include <cstdarg>
#include <cstdio> // vsnprintf

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

//  constexpr bool IsDebugMode()
//  {
//#ifdef NDEBUG
//    return false;
//#else
//    return true;
//#endif
//  }

  StackFrame::StackFrame( const int line,
                          const char* file,
                          const char* function )
  {
    const int i = StringView( file ).find_last_of( "/\\" );
    mLine = line;
    mFile = file + ( i == StringView::npos ? 0 : i + 1 );
    mFunction = function;
  }

  const char* StackFrame::ToString()  const
  {
    //SplitFilepath splitFilepath( mFile );
    //return va( "%s:%i %s", splitFilepath.mFilename.c_str(), mLine, mFunction );
    //SplitFilepath splitFilepath( mFile );
    return va( "%s:%i %s", mFile, mLine, mFunction );
  }

  void HandleAssert( const char* message, const StackFrame& frame )
  {
    Errors errors;
    errors.Append( message );
    errors.Append( frame );
    OSDebugAssert( errors );
  }

  int asdf = 0;
}
