#pragma once

#include "common/tacErrorHandling.h"
#include "common/tacString.h"
#include "common/containers/tacVector.h"

typedef TacVector< char > TacTemporaryMemory;

TacTemporaryMemory TacTemporaryMemoryFromFile( const TacStringView& path, TacErrors& errors );
TacTemporaryMemory TacTemporaryMemoryFromBytes( const void* bytes, int byteCount );

template< typename T >
TacTemporaryMemory TacTemporaryMemoryFromT( const T& t )
{
  return TacTemporaryMemoryFromBytes( ( void* )&t, ( int )sizeof( T ) );
}

void TacWriteToFile( const TacString& path,  void* bytes, int byteCount , TacErrors& errors );

//struct TacTemporaryMemoryRingBuffer
//{
//
//};

template< typename T >
struct TacOwned
{
  TacOwned() = default;
  TacOwned( T* t ) : mT( t ) {}
  ~TacOwned()
  {
    delete mT;
    mT = nullptr;
  }
  TacOwned& GainOwnershipFrom( TacOwned< T >& rhs )
  {
    delete mT;
    mT = rhs.mT;
    rhs.mT = nullptr;
    return *this;
  }
  TacOwned& operator = ( TacOwned< T >&& rhs )
  {
    return GainOwnershipFrom( rhs );
  }
  TacOwned& operator = ( TacOwned< T >& rhs )
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

