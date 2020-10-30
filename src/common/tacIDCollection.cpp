#include "common/tacIDCollection.h"
#include "common/tacMemory.h"

namespace Tac
{
  // sparse is an array keyed by a handle.
  // the value of the sparse array is an index into dense.
  //
  // the value of the dense array is the handle
  //
  // Example configurations of some handles
  //  - b(1)
  //  - a(0)
  //  - g(6)
  //
  //         0 1 2 3 4 5 6       0 1 2 3 4 5 6 
  //        +-+-+-+-+-+-+-+-+-  +-+-+-+-+-+-+-+-+-
  // dense  |b|a|g| | | | | |   |1|0|6| | | | | |
  //        +-+-+-+-+-+-+-+-+-  +-+-+-+-+-+-+-+-+-
  // sparse |1|0| | | | |2| |   |1|0| | | | |2| |
  //        +-+-+-+-+-+-+-+-+-  +-+-+-+-+-+-+-+-+-
  //         a b c d e f g h     a b c d e f g h
  //
  //         0 1 2 3 4 5 6       0 1 2 3 4 5 6 
  //        +-+-+-+-+-+-+-+-+-  +-+-+-+-+-+-+-+-+-
  // dense  |6|1|0| | | | | |   |0|6|1| | | | | |
  //        +-+-+-+-+-+-+-+-+-  +-+-+-+-+-+-+-+-+-
  // sparse |2|1| | | | |6| |   |0|1| | | | |1| |
  //        +-+-+-+-+-+-+-+-+-  +-+-+-+-+-+-+-+-+-
  //         a b c d e f g h     a b c d e f g h
  //

  IdCollection::IdCollection( const int capacity )
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

  void IdCollection::Free( const int id )
  {
    int* dense = GetDense();
    int* sparse = GetSparse();

    const int iRemoveSparse = id;
    const int iRemoveDense = sparse[ iRemoveSparse ];

    const int iLastDense = mSize - 1;
    const int iLastSparse = dense[ iLastDense ];

    TAC_ASSERT( ( unsigned )iRemoveDense < ( unsigned )mSize );

    dense[ iRemoveDense ] = iLastSparse;
    dense[ iLastDense ] = iRemoveSparse;
    sparse[ iLastSparse ] = iRemoveDense;
    sparse[ iRemoveSparse ] = iLastDense;

    --mSize;
  }

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
    IdCollection ids(5);

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
