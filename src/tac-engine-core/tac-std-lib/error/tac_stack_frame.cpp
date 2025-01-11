#include "tac_stack_frame.h" // self-inc
#include "tac-std-lib/string/tac_string_view.h"

namespace Tac
{
  StackFrame::StackFrame( const int line,
                          const char* file,
                          const char* function )
    : mLine     { line }
    , mFile     { file }
    , mFunction { function }
  {
  }

  int         StackFrame::GetLine() const     { return mLine; }
  const char* StackFrame::GetFile() const    
  {
    // Getting short file lazily to prioritize speed of stackframe creation
    const int i{ StringView( mFile ).find_last_of( "/\\" ) };
    return mFile + ( i == StringView::npos ? 0 : i + 1 );
  }
  const char* StackFrame::GetFunction() const { return mFunction; }
  bool        StackFrame::IsValid() const     { return mFile; }
}
