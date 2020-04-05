#pragma once

#include "src/common/tacString.h"
#include "src/common/containers/tacVector.h"
#include "src/common/tacPreprocessor.h"
namespace Tac
{


  struct Errors
  {
    void operator=( const StringView& message );
    void operator+=( const StringView& message );
    bool size() const;
    bool empty() const;
    void clear();
    operator bool() const;
    String ToString() const;
    //void Push( String message );
    //void Push( String message, Frame frame );
    //void Push( Frame frame );

    void Append( const Frame& frame );
    void Append( const StringView& message );

    String mMessage;
    Vector< Frame > mFrames;
  };

#define TAC_HANDLE_ERROR( errors )\
if( errors )\
{\
  errors.Append( TAC_FRAME );\
  return;\
}

}
