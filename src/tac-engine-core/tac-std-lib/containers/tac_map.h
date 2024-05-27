#pragma once

// hash map, collisions are cache-coherent via linear probing

#include "tac-std-lib/dataprocess/tac_hash.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/containers/tac_optional.h"

namespace Tac
{
  template< typename TKey, typename TVal >
  struct Map;

  void MapUnitTest();
}

template< typename TKey, typename TVal >
struct Tac::Map
{
  struct Pair
  {
    TKey mFirst{};
    TVal mSecond{};
  };

  struct Node
  {
    HashValue mHash     {};
    Pair      mPair     {};
    bool      mOccupied {};
  };

  struct ConstIterator
  {
    void  operator ++();
    const Pair& operator *() const            { return mCur->mPair; }

    bool  operator ==( const ConstIterator& ) const = default;
    operator bool() const                     { return mCur->mOccupied; }
    TVal GetValue() const                     { return mCur->mPair.mSecond; }

    const Node* mCur       {};
    int         mRemaining {};
  };

  struct Iterator
  {
    void  operator ++();
    Pair& operator *() const                  { return mCur->mPair; }

    bool  operator ==( const Iterator& ) const = default;
    operator bool() const                     { return mCur->mOccupied; }
    TVal GetValue() const                     { return mCur->mPair.mSecond; }

    Node* mCur       {};
    int   mRemaining {};
  };

  Map()                                       { mNodes = TAC_NEW Node[ mCapacity = 10 ]; }
  Map( const Map& other ) : Map()             { assign( other ); }
  ~Map()                                     
  {
    TAC_DELETE[] mNodes;
  }

  Iterator         begin();
  Iterator         end()                      { return {}; }
  ConstIterator    begin() const;
  ConstIterator    end() const                { return {}; }

  void             erase( TKey );
  bool             contains( TKey key ) const { return FindNode( key )->mOccupied; }
  void             Reserve( int );
  void             clear();
  int              size() const               { return mSize; }
  bool             empty() const              { return !mSize; }
  Iterator         Find( TKey );
  ConstIterator    Find( TKey ) const;
  Optional< TVal > FindVal( TKey ) const;
  void             insert_or_assign( TKey, TVal );
  TVal&            operator[]( TKey );
  void             operator = ( const Map& m ) { assign( m ); }

private:
  Node*            OccupyNode( TKey key );
  Node*            FindNode( TKey key )       { return &mNodes[ FindNodeIndex( key ) ]; }
  const Node*      FindNode( TKey key ) const { return &mNodes[ FindNodeIndex( key ) ]; }
  int              FindNodeIndex( TKey ) const;
  void             assign( const Map& other );

  int              mSize     {};
  int              mCapacity {};
  Node*            mNodes    {};
};



template< typename TKey, typename TVal >
Tac::Map< TKey, TVal >::ConstIterator Tac::Map< TKey, TVal >::begin() const
{
  if( !mSize )
    return {};

  ConstIterator iter{ mNodes, mSize - 1 };
  while( !iter.mCur->mOccupied )
    iter.mCur++;

  return iter;
}



template< typename TKey, typename TVal >
Tac::Map< TKey, TVal >::Iterator Tac::Map< TKey, TVal >::begin()
{
  if( !mSize )
    return {};

  Iterator iter{ mNodes, mSize - 1 };
  while( !iter.mCur->mOccupied )
    iter.mCur++;

  return iter;
}

template< typename TKey, typename TVal >
void Tac::Map< TKey, TVal >::erase( TKey key )
{
  Node* node { FindNode( key ) };
  *node = {};

  int iEmpty { int(node - mNodes) };
  int i { ( iEmpty + 1 ) % mCapacity };
  for( ;; )
  {
    node = &mNodes[ i ];
    if( !node->mOccupied )
      break;

    const int iDesired { (int)(node->mHash % mCapacity )};
    if( iDesired != i && iDesired <= iEmpty )
    {
      mNodes[ iEmpty ] = *node;
      *node = {};
      iEmpty = i;
    }

    i = ( i + 1 ) % mCapacity;
  }

  --mSize;
}

template< typename TKey, typename TVal >
Tac::Map<TKey, TVal>::Node* Tac::Map<TKey, TVal>::OccupyNode( TKey key )
{
  // We want to ensure that the array is at most half full, to better prevent collisiosn
  if( mSize + 1 > mCapacity / 2 )
    Reserve( mCapacity * 2 + 1 );

  if( Node* node{ FindNode( key ) }; node->mOccupied )
    return node;

  Node* node { FindNode( key ) };
  node->mOccupied = true;
  node->mHash = Tac::Hash( key );
  node->mPair.mFirst = key;
  mSize++;

  return node;
}

template< typename TKey, typename TVal >
int              Tac::Map<TKey, TVal>::FindNodeIndex( TKey key ) const
{
  const HashValue hash { Tac::Hash( key ) };
  int i{ ( int )( hash % mCapacity ) };
  for( ;; )
  {
    Node* node { &mNodes[ i ] };
    if( !node->mOccupied || ( node->mHash == hash && node->mPair.mFirst == key ) )
      return i;

    i = ( i + 1 ) % mCapacity;
  }
}



template< typename TKey, typename TVal >
void Tac::Map< TKey, TVal >::Reserve( int capacity )
{
  if( capacity < mCapacity )
    return;

  int oldCapacity { mCapacity };
  Node* oldNodes { mNodes };

  mNodes = TAC_NEW Node[ mCapacity = capacity ];
  mSize = 0;

  for( int i{}; i < oldCapacity; ++i )
    if( oldNodes[ i ].mOccupied )
      this->operator[]( oldNodes[i].mPair.mFirst ) =  oldNodes[i].mPair.mSecond;

  TAC_DELETE [] oldNodes;
}

template< typename TKey, typename TVal >
void             Tac::Map< TKey, TVal >::insert_or_assign( TKey key, TVal val )
{
  Node* node { OccupyNode( key ) };
  node->mPair.mSecond = val;
}

template< typename TKey, typename TVal >
TVal& Tac::Map< TKey, TVal >::operator[]( TKey key )
{
  Node* node { OccupyNode( key ) };
  return node->mPair.mSecond;
}

template< typename TKey, typename TVal >
void             Tac::Map<TKey, TVal>::assign( const Map& other )
{
  clear();
  for( const Pair& pair : other )
    insert_or_assign( pair.mFirst, pair.mSecond );
}


template< typename TKey, typename TVal >
void Tac::Map< TKey, TVal >::clear()
{
  mSize = 0;
  for( int i{}; i < mCapacity; ++i )
    mNodes[ i ] = {};
}


template< typename TKey, typename TVal >
Tac::Map< TKey, TVal >::Iterator Tac::Map< TKey, TVal >::Find( TKey key )
{
  Node* node { FindNode( key ) };
  return node->mOccupied ? Iterator{ .mCur = node } : Iterator{};
}

template< typename TKey, typename TVal >
Tac::Map< TKey, TVal >::ConstIterator    Tac::Map< TKey, TVal >::Find( TKey key ) const
{
  Node* node { FindNode( key ) };
  return node->mOccupied ? ConstIterator{ .mCur = node } : ConstIterator{};
}

template< typename TKey, typename TVal >
Tac::Optional< TVal > Tac::Map<TKey, TVal>::FindVal( TKey key ) const
{
  Node* node { &mNodes[ FindNodeIndex( key ) ] };
  return node->mOccupied ? Optional( node->mPair.mSecond ) : Optional<TVal>{};
}

template< typename TKey, typename TVal >
void Tac::Map<TKey, TVal>::Iterator::operator ++()
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

template< typename TKey, typename TVal >
void Tac::Map<TKey, TVal>::ConstIterator::operator ++()
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






