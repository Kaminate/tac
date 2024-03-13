#pragma once

#include "tac-std-lib/dataprocess/tac_hash.h"
#include "tac-std-lib/memory/tac_memory.h"

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
      HashValue mHash = 0;
      T         mT{};
      bool      mOccupied = false;
    };

    struct ConstIterator
    {
      bool        operator ==( const ConstIterator& ) const = default;
      void        operator ++();
      const T&    operator *()        { return mCur->mT; }

      const Node* mCur = nullptr;
      int         mRemaining = 0;
    };

    struct Iterator
    {
      bool     operator ==( const Iterator& ) const = default;
      void     operator ++();
      T&       operator *()        { return mCur->mT; }

      Node*    mCur = nullptr;
      int      mRemaining = 0;
    };

    Set()                            { mNodes = TAC_NEW Node[ mCapacity = 10 ]; }
    ~Set()                           { TAC_DELETE[] mNodes; }

    Iterator begin();
    Iterator end()                   { return {}; }
    ConstIterator begin() const;
    ConstIterator end() const        { return {}; }
    void     Remove( T );
    void     erase( T t )            { Remove( t ); }
    bool     Contains( T t ) const   { return FindNode( t )->mOccupied; }
    void     clear();
    int      size() const            { return mSize; }
    bool     empty() const           { return !mSize; }
    bool     insert( T );
    void     Reserve( int );

  private:
    Node*       FindNode( T t )       { return &mNodes[ FindNodeIndex(t) ]; }
    const Node* FindNode( T t ) const { return &mNodes[ FindNodeIndex(t) ]; }
    int         FindNodeIndex( T ) const;

    int      mSize = 0;
    int      mCapacity = 0;
    Node*    mNodes = nullptr;
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
    Node* node = FindNode( t );
    *node = {};
    int iEmpty = ( int )( node - mNodes );
    int i = ( iEmpty + 1 ) % mCapacity;
    for( ;; )
    {
      node = &mNodes[ i ];
      if( !node->mOccupied )
        break;

      const int iDesired = node->mHash % mCapacity;
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

  template< typename T>
  int Tac::Set<T>::FindNodeIndex( T t ) const
  {
    const HashValue hash = Tac::Hash( t );
    int i = hash % mCapacity;
    for( ;; )
    {
      Node* node = &mNodes[ i ];
      if( !node->mOccupied || ( node->mHash == hash && node->mT == t ) )
        return i;

      i = ( i + 1 ) % mCapacity;
    }

    return -1;
  }


  template< typename T>
  bool Tac::Set<T>::insert( T t )
  {
    Node* node = FindNode( t );
    if( node->mOccupied )
      return false;

    *node = Node
    {
      .mHash = Tac::Hash( t ),
      .mT = t,
      .mOccupied = true,
    };

    mSize++;
    return true;
  }

  template< typename T>
  void Tac::Set< T>::Reserve( int capacity )
  {
    if( capacity < mCapacity )
      return;

    int oldCapacity = mCapacity;
    Node* oldNodes = mNodes;

    mSize = 0;
    mNodes = TAC_NEW Node[ mCapacity = capacity ];

    for( int i = 0; i < oldCapacity; ++i )
      if( oldNodes[i].mOccupied )
        insert( oldNodes[i].mT );

    TAC_DELETE [] oldNodes;
  }


  template< typename T>
  void Tac::Set< T>::clear()
  {
    mSize = 0;
    for( int i = 0; i < mCapacity; ++i )
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

