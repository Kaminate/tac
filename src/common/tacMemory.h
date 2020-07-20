#pragma once

#include "src/common/tacErrorHandling.h"
#include "src/common/tacString.h"
#include "src/common/containers/tacVector.h"


void* operator new ( std::size_t size );
//void* operator new( std::size_t, Tac::StackFrame );
//#define TAC_NEW new(TAC_STACK_FRAME)
//
//void operator delete( void* ) noexcept;
//#define TAC_DELETE delete

namespace Tac
{


  typedef Vector< char > TemporaryMemory;

  TemporaryMemory TemporaryMemoryFromFile( const StringView& path, Errors& errors );
  TemporaryMemory TemporaryMemoryFromBytes( const void* bytes, int byteCount );
  String FileToString( const StringView& path, Errors& errors );

  template< typename T >
  TemporaryMemory TemporaryMemoryFromT( const T& t )
  {
    return TemporaryMemoryFromBytes( ( void* )&t, ( int )sizeof( T ) );
  }

  void WriteToFile( const String& path, void* bytes, int byteCount, Errors& errors );

  //struct TemporaryMemoryRingBuffer
  //{
  //
  //};


  //template <typename T, typename... Args>
  //T* New(StackFrame stackFrame, Args&&... args)
  //{
  //    // log ...
  //    return new T { std::forward<Args>(args)... };
  //}

  void SetNewStackFrame( StackFrame );

}

#define TAC_NEW SetNewStackFrame( TAC_STACK_FRAME ), new

