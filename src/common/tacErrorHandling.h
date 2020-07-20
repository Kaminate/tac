#pragma once

#include "src/common/tacString.h"
#include "src/common/containers/tacVector.h"
#include "src/common/tacPreprocessor.h"
namespace Tac
{
  struct Errors
  {
    void operator=( StringView message );
    void operator+=( StringView message );
    bool size() const;
    bool empty() const;
    void clear();
    operator bool() const;
    String ToString() const;

    void Append( const StackFrame& frame );
    void Append( StringView message );

    String mMessage;
    Vector< StackFrame > mFrames;
  };

#define TAC_HANDLE_ERROR( errors )\
if( errors )\
{\
  errors.Append( TAC_STACK_FRAME );\
  return;\
}

#define TAC_HANDLE_ERROR_IF( pred, msg, errors )\
if( pred )\
{\
  errors.Append( msg );\
  errors.Append( TAC_STACK_FRAME );\
  return;\
}

}
