#include "common/tacIDCollection.h"
#include "common/tacMemory.h"

namespace Tac
{
  void IdCollection::Init( int capacity )
  {
    mData = TAC_NEW int[ capacity * 2 ];
    mCapacity = capacity;
    int* dense = GetDense();
    int* sparse = GetSparse();
    for( int i = 0; i < capacity; ++i )
    {
      dense[ i ] = i;
      sparse[ i ] = i;
    }
  }

  void IdCollection::Uninit()
  {
    delete[] mData;
    mCapacity = 0;
    mData = nullptr;
    mSize = 0;
  }

  int* IdCollection::GetDense()
  {
    return mData;
  }

  int* IdCollection::GetSparse()
  {
    return mData + mCapacity;
  }

  int IdCollection::Alloc()
  {
    int* dense = GetDense();
    int* sparse = GetSparse();

    TAC_ASSERT( mSize < mCapacity );
    const int iDense = mSize++;
    const int iSparse = dense[ iDense ];
    sparse[ iSparse ] = iDense;
    return iSparse;
  }

  void IdCollection::Free( int id )
  {
    int* dense = GetDense();
    int* sparse = GetSparse();
    const int iDense = sparse[ id ];
    const int iSparseNew = dense[ --mSize ];
    dense[ iDense ] = iSparseNew;
    sparse[ iSparseNew ] = iDense;
  }
  //int             Size();
  //int             Lookup( int index );

  int* IdCollection::begin()
  {
    return GetDense();
  }

  int* IdCollection::end()
  {
    return GetDense() + mSize;
  }
  
  void UnitTestIdCollection()
  {
    IdCollection ids;
    ids.Init( 5 );

    const int id0 = ids.Alloc();
    TAC_ASSERT( id0 == 0 );

    const int id1 = ids.Alloc();
    TAC_ASSERT( id0 == 1 );

    const int id2 = ids.Alloc();
    TAC_ASSERT( id0 == 2 );

    ids.Free( id0 );





    ids.Uninit();
  }
}
