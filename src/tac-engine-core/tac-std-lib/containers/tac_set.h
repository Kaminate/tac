#pragma once

#include "tac-std-lib/dataprocess/tac_hash.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/error/tac_assert.h"

#include "tac-std-lib/preprocess/tac_preprocessor.h" // TAC_DELETE_ME
#if !TAC_DELETE_ME()
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/tac_type_traits.h"
#endif

#define TAC_SET_STL() 1

#if TAC_SET_STL()

#if TAC_SHOULD_IMPORT_STD()
import std;
#else
#include <set>
#endif

namespace Tac
{
  template< typename T >
  struct Set : public std::set< T >
  {
    bool Contains( const T& t ) const { return this->contains( t ); }
  };
  void SetUnitTest();
}
#else

// hash map, collisions are cache-coherent via linear probing

namespace Tac
{
  template< typename T >
  struct Set;

  void SetUnitTest();
}

template< typename T >
struct Tac::Set
{
  struct Node
  {
    HashValue mHash     {};
    T         mT        {};
    bool      mOccupied {};
  };

  struct ConstIterator
  {
    bool        operator ==( const ConstIterator& ) const = default;
    void        operator ++();
    const T&    operator *()            { return mCur->mT; }

    const Node* mCur       {};
    int         mRemaining {};
  };

  struct Iterator
  {
    bool     operator ==( const Iterator& ) const = default;
    void     operator ++();
    T&       operator *()               { return mCur->mT; }

    Node*    mCur       {};
    int      mRemaining {};
  };

  Set()                                 { mNodes = TAC_NEW Node[ mCapacity = 10 ]; }
  ~Set()                                { TAC_DELETE[] mNodes; }

  Set( Set&& other )
  {
    TAC_ASSERT( false );
  }

  Set( const Set& other )
  {
    TAC_ASSERT( false );
  }

  void operator = ( Set&& other )
  {
    TAC_ASSERT( false );
  }

  void operator = ( const Set& other )
  {
    TAC_ASSERT( false );
  }

  Iterator      begin();
  Iterator      end()                   { return {}; }
  ConstIterator begin() const;
  ConstIterator end() const             { return {}; }
  void          Remove( T );
  void          erase( T t )            { Remove( t ); }
  bool          Contains( T t ) const   { return FindNode( t )->mOccupied; }
  void          clear();
  int           size() const            { return mSize; }
  bool          empty() const           { return !mSize; }
  bool          insert( T );
  void          Reserve( int );

private:
  Node*         FindNode( T t )         { return &mNodes[ FindNodeIndex(t) ]; }
  const Node*   FindNode( T t ) const   { return &mNodes[ FindNodeIndex(t) ]; }
  int           FindNodeIndex( T ) const;
  int           BaseNodeIndex( HashValue n ) const { return int( unsigned( n ) % mCapacity ); }



  void TEMP_PRINT()
  {
    if( kIsPointer< T > )
    {
      OS::OSDebugPrintLine( "----- SET BEGIN --------" );
      OS::OSDebugPrintLine( String() + "Capacity: " + ToString( mCapacity ) );
      OS::OSDebugPrintLine( String() + "Size: " + ToString( mSize ) );
      for( int i{}; i < mCapacity; ++i )
      {
        Node& node{mNodes[i]};
        node.mHash;
        node.mOccupied;
        node.mT;

        String s;
        s += "node " + ToString( i ) + ": ";
        s += "(occupied: " + String(node.mOccupied ? "true " : "false") + ")";
        if( node.mOccupied )
        {
          s += "(hash: " + ToString( node.mHash ) + ")";
          s += "(base hash: " + ToString( BaseNodeIndex(node.mHash ) ) + ")";
          s += "(T: " + ToString( node.mT ) + ")";
        }
        OS::OSDebugPrintLine(s);
      }
      
      OS::OSDebugPrintLine( "----- SET END   --------" );

      
    }
  }

  void TEMP_ASSERT_OCCUPY()
  {
    int nOccupied{};
    for( int i{}; i < mCapacity; ++i )
    {

      Node& node{ mNodes[ i ] };
      if( node.mOccupied )
        ++nOccupied;
    }

    TAC_ASSERT(nOccupied == mSize);
  }

  int           mSize     {};
  int           mCapacity {};
  Node*         mNodes    {};
};

template< typename T>
Tac::Set< T>::Iterator Tac::Set< T>::begin()
{
  if( !mSize )
    return {};

  Iterator iter{ mNodes, mSize - 1 };
  while( !iter.mCur->mOccupied )
    iter.mCur++;

  return iter;
}

template< typename T>
Tac::Set< T>::ConstIterator Tac::Set< T>::begin() const
{
  if( !mSize )
    return {};

  ConstIterator iter{ mNodes, mSize - 1 };
  while( !iter.mCur->mOccupied )
    iter.mCur++;

  return iter;
}


template< typename T>
void Tac::Set< T>::Remove( T t )
{
  //TEMP_ASSERT_OCCUPY();
  //TEMP_PRINT();




  Node* node{ FindNode( t ) };
  *node = {};
  --mSize;
  int iEmpty{ ( int )( node - mNodes ) };
  int i{ ( iEmpty + 1 ) % mCapacity };
  for( ;; )
  {
    node = &mNodes[ i ];
    if( !node->mOccupied )
      break;

    //const int iDesired{ ( unsigned )node->mHash % mCapacity };
    const int iDesired{ BaseNodeIndex( node->mHash ) };
    if( iDesired != i && iDesired <= iEmpty )
    {
      mNodes[ iEmpty ] = *node;
      *node = {};
      iEmpty = i;
    }

    i = ( i + 1 ) % mCapacity;
  }


  //TEMP_ASSERT_OCCUPY();
}

template< typename T>
int Tac::Set<T>::FindNodeIndex( T t ) const
{
  const HashValue hash{ Tac::Hash( t ) };
  int i{ BaseNodeIndex( hash ) };
  //int i{ ( int )( hash % mCapacity ) };
  for( ;; )
  {
    const Node* node{ &mNodes[ i ] };
    if( !node->mOccupied )
      return i;

    if( node->mHash == hash && node->mT == t )
      return i;

    i = ( i + 1 ) % mCapacity;
  }

}


template< typename T>
bool Tac::Set<T>::insert( T t )
{
  // We want to ensure that the array is at most half full, to better prevent collisiosn
  if( mSize + 1 > mCapacity / 2 )
    Reserve( mCapacity * 2 + 1 );

  Node* node { FindNode( t ) };
  if( node->mOccupied )
    return false;

  *node = Node
  {
    .mHash     { Tac::Hash( t ) },
    .mT        { t },
    .mOccupied { true },
  };

  mSize++;
  return true;
}

template< typename T>
void Tac::Set< T>::Reserve( int capacity )
{
  if( capacity < mCapacity )
    return;

  int oldCapacity { mCapacity };
  Node* oldNodes { mNodes };

  mSize = 0;
  mNodes = TAC_NEW Node[ mCapacity = capacity ];

  for( int i{}; i < oldCapacity; ++i )
    if( oldNodes[ i ].mOccupied )
      insert( oldNodes[ i ].mT );

  TAC_DELETE[] oldNodes;
}


template< typename T>
void Tac::Set< T>::clear()
{
  mSize = 0;
  for( int i{}; i < mCapacity; ++i )
    mNodes[ i ] = {};
}

template< typename T>
void Tac::Set<T>::Iterator::operator ++()
{
  if( !mRemaining )
  {
    mCur = nullptr;
    return;
  }

  mCur++;
  while( !mCur->mOccupied )
    mCur++;

  mRemaining--;
}

template< typename T>
void Tac::Set<T>::ConstIterator::operator ++()
{
  if( !mRemaining )
  {
    mCur = nullptr;
    return;
  }

  mCur++;
  while( !mCur->mOccupied )
    mCur++;

  mRemaining--;
}

#endif
