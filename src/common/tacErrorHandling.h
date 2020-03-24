#pragma once

#include "tacString.h"
#include "containers/tacVector.h"
#include "tacPreprocessor.h"

struct TacErrors
{
  void operator=( const TacString& message );
  void operator+=( const TacString& message );
  bool size() const;
  bool empty() const;
  void clear();
  TacString ToString() const;
  //void Push( TacString message );
  //void Push( TacString message, TacStackFrame stackFrame );
  //void Push( TacStackFrame stackFrame );

  void Append( const TacStackFrame& stackFrame );
  void Append( const TacString& message );

  TacString mMessage;
  TacVector< TacStackFrame > mStackFrames;
};

#define TAC_HANDLE_ERROR( errors )\
if( ( errors.size() ) )\
{\
  errors.Append( TAC_STACK_FRAME );\
  return;\
}

