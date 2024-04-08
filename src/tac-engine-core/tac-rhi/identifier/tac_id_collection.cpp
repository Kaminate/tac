#include "tac_id_collection.h" // self-inc

#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"

namespace Tac
{
  // A Tac::IdCollection allocates Ids.
  // Each Id has a corresponding dense and sparse array element, which point to each other.
  //
  // The dense array contains allocated ids on the left, and free ids on the right.
  // The sparse array is used so that when you free an id, it knows how to move that id from
  // the allocated section of the dense array to the free list section of the dense array.
  //
  // <---- mSize ---->
  // [ allocated ids ][       free ids         ][                                          ]
  // <-------------- dense array --------------><-------------- sparse array -------------->
  //
  // Allocated handle values are sparse array indexes 

  IdCollection::~IdCollection()
  {
    TAC_DELETE[] mDense;
  }

  void IdCollection::reserve( const int capacity )
  {
    TAC_ASSERT( capacity > mCapacity );

    int* newDense = TAC_NEW int[ ( size_t )capacity * 2 ];
    int* newSparse = newDense + capacity;

    for( int i = 0; i < mCapacity; ++i )
    {
      newDense[ i ] = mDense[ i ];
      newSparse[ i ] = mSparse[ i ];
    }

    for( int i = mCapacity; i < capacity; ++i )
    {
      newDense[ i ] = i;
      newSparse[ i ] = i;
    }

    TAC_DELETE[] mDense;
    mDense = newDense;
    mSparse = newSparse;
    mCapacity = capacity;
  }

  int IdCollection::Alloc()
  {
    if( mSize == mCapacity )
      reserve( mCapacity ? int( mCapacity * 1.5f ) : 5 );

    const int iDense = mSize++;
    const int iSparse = mDense[ iDense ]; 

    TAC_ASSERT( mSparse[ iSparse ] == iDense  );
    return iSparse;
  }

  void IdCollection::Free( const int id )
  {
    const int iRemoveSparse = id;
    const int iRemoveDense = mSparse[ iRemoveSparse ];
    TAC_ASSERT_INDEX( iRemoveDense, mSize );

    const int iLastDense = --mSize;
    const int iLastSparse = mDense[ iLastDense ];

    mDense[ iRemoveDense ] = iLastSparse;
    mDense[ iLastDense ] = iRemoveSparse;
    mSparse[ iLastSparse ] = iRemoveDense;
    mSparse[ iRemoveSparse ] = iLastDense;
  }

  
  void UnitTestIdCollection()
  {
    IdCollection ids;

    const int id0 = ids.Alloc();
    TAC_ASSERT( id0 == 0 );

    const int id1 = ids.Alloc();
    TAC_ASSERT( id0 == 1 );

    const int id2 = ids.Alloc();
    TAC_ASSERT( id0 == 2 );

    ids.Free( id0 );
  }
}
