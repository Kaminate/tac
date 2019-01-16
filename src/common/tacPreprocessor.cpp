#include "tacPreprocessor.h"
#include "tacUtility.h"
#include "tacOS.h"
#include <cstdarg>


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

bool TacIsDebugMode()
{
#ifdef NDEBUG
  return false;
#else
  return true;
#endif
}


TacString TacStackFrame::ToString()  const
{
    TacSplitFilepath splitFilepath( mFile );
    TacString pathString = splitFilepath.mFilename;
    TacString lineString = TacToString( mLine );
    TacString result = pathString + ":" + lineString + " " + mFunction;
    return result;
}

void TacAssertInternal( const TacString& message, const TacStackFrame& stackFrame )
{
  TacOS::Instance->DebugAssert( message, stackFrame );
}
