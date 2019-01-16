#pragma once

#include "common/tacErrorHandling.h"
//#include "common/tacPreprocessor.h"
#include "common/tacString.h"
#include "common/containers/tacVector.h"

TacVector< char > TacTemporaryMemory( const TacString& path, TacErrors& errors );
TacVector< char > TacTemporaryMemory( void* bytes, int byteCount );
template< typename T >
TacVector< char > TacTemporaryMemory( const T& t ) { return TacTemporaryMemory( ( void* )&t, ( int )sizeof( T ) ); }
void TacWriteToFile( const TacString& path,  void* bytes, int byteCount , TacErrors& errors );


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

