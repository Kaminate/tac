// This file is responsible for overloading the global new and delete operators.
// Allocate through TAC_NEW/TAC_DELETE instead of new/delete to route through the memory manager
#pragma once

#if defined( _MSC_VER )
#pragma warning( disable: 28251 ) // inconsistant annotation for new
#endif // #if defined( _MSC_VER )

#include "tac-std-lib/error/tac_stack_frame.h"
#include "tac-std-lib/tac_ints.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <memory>
#endif

//import std; // std::size_t
//#include <cstddef> // std::size_t

namespace Tac
{
  // Inform the memory manager which line of code is about to allocate memory
  void SetNewStackFrame( const StackFrame& );

  // Proof of concept for overloading operator new/delete
  // This can be replaced with stuff like kAllocationContext, kAlignment
  struct Happy
  {
    Happy( int ii ) : i( ii ) {}
    int i;
  };

  static const Happy kHappyBoi( 1 );
  static const Happy kHappyGrl( 2 );

  void* Allocate( std::size_t, StackFrame );
  void* Allocate( std::size_t );
  void Deallocate( void* );
}


// Example use: mBytes = TAC_NEW char[ byteCount ];
void* operator new ( std::size_t );

// Example use: args.mElements.push_back( TAC_NEW( kHappyGril ) Json( s ) );
void* operator new ( std::size_t, Tac::Happy );

// Each new must be paired with a delete
void operator delete( void* ) noexcept;
void operator delete( void*, std::size_t ) noexcept; // and a sized delete?
void operator delete( void*, Tac::Happy ) noexcept;



#define TAC_NEW ( Tac::SetNewStackFrame( TAC_STACK_FRAME ), false ) ? nullptr : new
#define TAC_PLACEMENT_NEW new
#define TAC_DELETE delete


