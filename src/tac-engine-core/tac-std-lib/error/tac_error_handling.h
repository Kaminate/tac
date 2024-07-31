#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_span.h"
#include "tac-std-lib/error/tac_stack_frame.h"

#undef GetMessage

namespace Tac
{
  struct Errors
  {
    enum Flags
    {
      kNone        = 0b0000,
      kDebugBreaks = 0b0001,
    };

    Errors() = default;
    Errors( Flags );

    operator bool() const;

    bool                 empty() const;
    void                 clear();
    void                 Raise( StringView, StackFrame );
    void                 Propagate( StackFrame );
    Span< const StackFrame >   GetFrames() const;
    StringView           GetMessage() const;
    String               ToString() const;

  private:

    String               mMessage {};
    Vector< StackFrame > mFrames  {};
    Flags                mFlags   {};
    bool                 mBroken  {};
  };

} // namespace Tac

// Raise an error, returning from the function
#define TAC_RAISE_ERROR( msg )                       { errors.Raise( msg, TAC_STACK_FRAME ); return; }
#define TAC_RAISE_ERROR_RETURN( ret, msg )           { errors.Raise( msg, TAC_STACK_FRAME ); return ret; }
#define TAC_RAISE_ERROR_IF( pred, msg)               if( pred ){ TAC_RAISE_ERROR( msg ); }
#define TAC_RAISE_ERROR_IF_RETURN( ret, pred, msg )  if( pred ){ TAC_RAISE_ERROR_RETURN( ret , msg ); }

// Call a function, and upon errors propagates the stack frame
#define TAC_CALL( call )                             call; if( errors ){ errors.Propagate( TAC_STACK_FRAME ); return; }
#define TAC_CALL_RET( ret, call )                    call; if( errors ){ errors.Propagate( TAC_STACK_FRAME ); return ret; }

