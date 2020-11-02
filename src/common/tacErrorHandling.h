#pragma once

#include "src/common/tacString.h"
#include "src/common/containers/tacVector.h"
#include "src/common/tacPreprocessor.h"
namespace Tac
{
  struct Errors
  {
    enum Flags
    {
      kNone = 0b0000,
      kDebugBreakOnAppend = 0b0001,
      // Append stack frame?
      // Append messages?
    };
    Errors( Flags flags = kNone );
    bool                 size() const;
    bool                 empty() const;
    void                 clear();
    String               ToString() const;
    void                 Append( const StackFrame& frame );
    void                 Append( const StringView& message );
    void                 OnAppend();
    //void                 operator=( const char* );
    //void                 operator=( StringView );
    //void                 operator+=( StringView );
    operator bool() const;
    String               mMessage;
    Vector< StackFrame > mFrames;
    Flags                mFlags;
    bool                 mBroken;
  };

#define TAC_RAISE_ERROR( msg, errors )           \
{                                                \
  errors.Append( msg );                          \
  errors.Append( TAC_STACK_FRAME );              \
  return;                                        \
}

#define TAC_HANDLE_ERROR( errors )               \
if( errors )                                     \
{                                                \
  errors.Append( TAC_STACK_FRAME );              \
  return;                                        \
}

#define TAC_HANDLE_ERROR_IF( pred, msg, errors ) if( pred ) { TAC_RAISE_ERROR( msg, errors ); }

}
