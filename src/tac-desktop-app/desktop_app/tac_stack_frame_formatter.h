#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/containers/tac_span.h"
#include "tac-std-lib/error/tac_stack_frame.h"

namespace Tac
{
  
  struct StackFrameFormatter
  {
    StackFrameFormatter( Span< const StackFrame > );
    String FormatFrames() const;

  private:
    String FormatFrame( StackFrame ) const;
    String FmtLen( StringView, int ) const;
    String FmtFile( StackFrame ) const;
    String FmtLine( StackFrame ) const;

    int                      mMaxLenFilename {};
    int                      mMaxLenLine     {};
    Span< const StackFrame > mFrames         {};
  };


} // namespace Tac
