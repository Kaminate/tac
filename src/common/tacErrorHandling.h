#pragma once

#include "tacString.h"
#include "containers/tacVector.h"
#include "tacPreprocessor.h"

struct TacErrors
{
  void operator=( const TacStringView& message );
  void operator+=( const TacStringView& message );
  bool size() const;
  bool empty() const;
  void clear();
  operator bool() const;
  TacString ToString() const;
  //void Push( TacString message );
  //void Push( TacString message, TacStackFrame stackFrame );
  //void Push( TacStackFrame stackFrame );

  void Append( const TacStackFrame& stackFrame );
  void Append( const TacStringView& message );

  TacString mMessage;
  TacVector< TacStackFrame > mStackFrames;
};

#define TAC_HANDLE_ERROR( errors )\
if( ( errors ) )\
{\
  errors.Append( TAC_STACK_FRAME );\
  return;\
}

