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

template< typename T >
struct Owned
{
  Owned() = default;
  Owned( T* t ) : mT( t ) {}
  ~Owned()
  {
    delete mT;
    mT = nullptr;
  }
  Owned& GainOwnershipFrom( Owned< T >& rhs )
  {
    delete mT;
    mT = rhs.mT;
    rhs.mT = nullptr;
    return *this;
  }
  Owned& operator = ( Owned< T >&& rhs )
  {
    return GainOwnershipFrom( rhs );
  }
  Owned& operator = ( Owned< T >& rhs )
  {
    return GainOwnershipFrom( rhs );
  }
  T* operator = ( T* t )
  {
    delete mT;
    mT = t;
    return t;
  }
  operator T*( ) { return mT; }
  T* operator ->() { return mT; }
  T* mT = nullptr;
};

}
