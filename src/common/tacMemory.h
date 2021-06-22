// This file is responsible for overloading the global new and delete operators.
// Allocate through TAC_NEW/TAC_DELETE instead of new/delete to route through the memory manager
#pragma once

#if _MSC_VER
#pragma warning( disable: 28251 ) // inconsistant annotation for new
#endif

#include "src/common/tacPreprocessor.h"
#include <cstddef> // std::size_t

namespace Tac
{
  // Inform the memory manager which line of code is about to allocate memory
  void SetNewStackFrame( StackFrame );

  // Proof of concept for overloading operator new/delete
  // This can be replaced with stuff like kAllocationContext, kAlignment
  struct Happy
  {
    Happy( int ii ) : i( ii ) {}
    int i;
  };

  static const Happy kHappyBoi(1);
  static const Happy kHappyGrl(2);
}


// Example use: mBytes = TAC_NEW char[ byteCount ];
void* operator new ( std::size_t );

// Example use: args.mElements.push_back( TAC_NEW( kHappyGril ) Json( s ) );
void* operator new ( std::size_t, Tac::Happy );

// Each new must be paired with a delete
void operator delete( void* ) noexcept;
void operator delete( void*, Tac::Happy ) noexcept;

#define TAC_NEW ( Tac::SetNewStackFrame( TAC_STACK_FRAME ), false ) ? nullptr : new
#define TAC_DELETE delete


