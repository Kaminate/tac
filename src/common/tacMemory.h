#pragma once

#include "src/common/tacErrorHandling.h"
#include "src/common/tacString.h"
#include "src/common/containers/tacVector.h"

namespace Tac
{


typedef Vector< char > TemporaryMemory;

TemporaryMemory TemporaryMemoryFromFile( const StringView& path, Errors& errors );
TemporaryMemory TemporaryMemoryFromBytes( const void* bytes, int byteCount );

template< typename T >
TemporaryMemory TemporaryMemoryFromT( const T& t )
{
  return TemporaryMemoryFromBytes( ( void* )&t, ( int )sizeof( T ) );
}

void WriteToFile( const String& path,  void* bytes, int byteCount , Errors& errors );

//struct TemporaryMemoryRingBuffer
//{
//
//};

}
