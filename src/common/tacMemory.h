#pragma once

//#include "src/common/tacErrorHandling.h"
//#include "src/common/tacString.h"
#include "src/common/tacPreprocessor.h"
#include <cstddef> // std::size_t

namespace Tac
{
  void SetNewStackFrame( StackFrame );
}

// Overload of global new and delete
void operator delete( void* ) noexcept;
void* operator new ( std::size_t );

// wow c++ macros
#define TAC_NEW ( Tac::SetNewStackFrame( TAC_STACK_FRAME ), false ) ? nullptr : new
#define TAC_DELETE delete


