#include "tac_stack_frame.h" // self-inc
#include "tac-std-lib/string/tac_string_view.h"

namespace Tac
{
  StackFrame::StackFrame( const int line,
                          const char* file,
                          const char* function )
  {
    const int i{ StringView( file ).find_last_of( "/\\" ) };
    mLine = line;
    mFile = file + ( i == StringView::npos ? 0 : i + 1 );
    mFunction = function;
  }

  int         StackFrame::GetLine() const     { return mLine; }
  const char* StackFrame::GetFile() const     { return mFile; }
  const char* StackFrame::GetFunction() const { return mFunction; }
  bool        StackFrame::IsValid() const     { return mFile; }
}
