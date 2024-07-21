#pragma once

#include "tac-std-lib/memory/tac_memory.h"

namespace Tac
{
  template< typename T > struct ForwardList;

  void ForwardListUnitTest();
}


// Why are you using a list?
template< typename T >
struct Tac::ForwardList
{
private:
  struct Node
  {
    ~Node()                { TAC_DELETE mNext; mNext = nullptr; }

    T        mT    {};
    Node*    mNext {};
  };

public:
  struct ConstIterator
  {
    operator    bool()        { return mNode; }
    const T&    operator *()  { return mNode->mT; }
    void        operator ++() { mNode = mNode->mNext; }

    const Node* mNode {};
  };

  struct Iterator
  {
    operator    bool()        { return mNode; }
    T&          operator *()  { return mNode->mT; }
    void        operator ++() { mNode = mNode->mNext; }

    Node*       mNode {};
  };

  ~ForwardList()           { Clear(); }
  ForwardList() = default;
  ForwardList( const ForwardList& l ) { assign( l ); }
  void            PushFront(T);
  T               PopFront();
  Iterator        begin()       { return { mHead }; }
  Iterator        end()         { return {}; }
  ConstIterator   begin() const { return { mHead }; }
  ConstIterator   end() const   { return {}; }
  bool            empty() const { return !mSize; }
  int             size() const  { return mSize; }
  void            Clear()       { TAC_DELETE mHead; mHead = nullptr; mSize = 0; }
  void            operator = ( const ForwardList& l ) { assign( l ); }

private:

  void            assign( const ForwardList& );

  Node*           mHead {};
  int             mSize {};
};

template< typename T >
void       Tac::ForwardList<T>::assign( const ForwardList& other )
{
  Clear();
  ForwardList tmp;
  for( const T& t : other )
    tmp.PushFront( t );
  for( const T& t : tmp )
    PushFront( t );
}

template< typename T >
void Tac::ForwardList<T>::PushFront( T t )
{
  mHead = TAC_NEW Node{ .mT = t, .mNext = mHead };
  mSize++;
}

template< typename T >
T Tac::ForwardList<T>::PopFront()
{
  T t { mHead->mT };
  mSize--;
  mHead = mHead->mNext;
  return t;
}

