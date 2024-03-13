#include "tac_stack_frame.h" // self-inc
//#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-std-lib/string/tac_string_view.h"

namespace Tac
{
  StackFrame::StackFrame( const int line,
                          const char* file,
                          const char* function )
  {
    const int i = StringView( file ).find_last_of( "/\\" );
    mLine = line;
    mFile = file + ( i == StringView::npos ? 0 : i + 1 );
    mFunction = function;
  }

  //const char* StackFrame::ToString()  const
  //{
  //  return FrameMemoryCopy(
  //    String()
  //    + mFile + ":"
  //    + Tac::ToString( mLine )
  //    + " "
  //    + mFunction + "()");
  //}
}
