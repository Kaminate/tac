#pragma once

#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/error/tac_assert.h"

namespace Tac
{
  template< typename T > struct List;

  void ListUnitTest();
}


// Why are you using a list?
template< typename T >
struct Tac::List
{
private:
  struct Node
  {
    T        mT    {};
    Node*    mPrev {};
    Node*    mNext {};
  };

public:


  struct ConstIterator
  {
    operator      bool()        { return mNode; }
    bool          operator == ( const ConstIterator& ) const = default;
    const T&      operator *()  { return mNode->mT; }
    dynmc T*      operator ->() { return &mNode->mT; }
    void          operator ++() { mNode = mNode->mNext; }
    void          operator --() { mNode = mNode->mPrev; } // pre-dec
    ConstIterator operator --(int) // post-dec
    {
      ConstIterator it { *this };
      mNode = mNode->mPrev;
      return it;
    } 

    const Node*   mNode {};
  };

  struct Iterator
  {
    operator    bool()        { return mNode; }
    bool        operator == ( const Iterator& ) const = default;
    dynmc T&    operator *()  { return mNode->mT; }
    dynmc T*    operator ->() { return &mNode->mT; }
    void        operator ++() { mNode = mNode->mNext; }
    void        operator --() { mNode = mNode->mPrev; } // pre-dec
    Iterator    operator --(int) // post-dec
    {
      Iterator it { *this };
      mNode = mNode->mPrev;
      return it;
    } 

    dynmc Node* mNode {};
  };

  List() = default;
  //List()                                     { mDummy.mNext = &mDummy; mDummy.mPrev = &mDummy; }
  List(const List& other) : List()           { assign( other ); }
  ~List()                                    { clear(); }


  void          push_front( T t )             { InsertAfter( &mDummy, t ); }
  void          push_back( T t )              { InsertBefore( &mDummy, t ); }
  T             pop_front();
  T             pop_back();
  T&            front()                      { return mDummy.mNext->mT; }
  const T&      front() const                { return mDummy.mNext->mT; }
  T&            back()                       { return mDummy.mPrev->mT; }
  const T&      back() const                 { return mDummy.mPrev->mT; }
  Iterator      begin()                      { return { mDummy.mNext }; }
  ConstIterator begin() const                { return { mDummy.mNext }; }
  Iterator      rbegin()                     { return { mDummy.mPrev }; }
  ConstIterator rbegin() const               { return { mDummy.mPrev }; }
  Iterator      end()                        { return { &mDummy }; }
  ConstIterator end() const                  { return { &mDummy }; }
  Iterator      Find( T t )                  { return { FindNode( t ) }; }
  ConstIterator Find( T t ) const            { return { FindNode( t ) }; }
  bool          empty() const                { return !mSize; }
  int           size() const                 { return mSize; }
  void          clear();
  Iterator      erase( Iterator it ); // returns the next iterator after `it`
  void          operator = ( const List& l ) { assign( l ); }

private:

  void          assign( const List& );
  Node*         FindNode( T );
  void          EraseNode( Node* );
  void          InsertAfter( Node* n, T t )  { InsertBetween( n, n->mNext, t ); }
  void          InsertBefore( Node* n, T t ) { InsertBetween( n->mPrev, n, t ); }
  void          InsertBetween( Node*, Node*, T );

  Node          mDummy{ .mPrev = &mDummy, .mNext = &mDummy };
  int           mSize {  };
};


template< typename T >
void          Tac::List<T>::assign( const List& other )
{
  clear();
  for( const T& t : other )
    push_back( t );
}

template< typename T >
T          Tac::List<T>::pop_front()
{
  TAC_ASSERT( !empty() );
  const T t { mDummy.mNext->mT };
  EraseNode( mDummy.mNext );
  return t;
}

template< typename T >
T          Tac::List<T>::pop_back()
{
  TAC_ASSERT( !empty() );
  T t { mDummy.mPrev.mT };
  EraseNode( mDummy.mPrev );
  return t;
}

template< typename T >
Tac::List<T>::Node* Tac::List<T>::FindNode( T t )
{
  for( Node* node { mDummy.mNext }; node != &mDummy; node = node->mNext )
    if( node->mT == t )
      return node;
  return nullptr;
}

template< typename T >
Tac::List<T>::Iterator      Tac::List<T>::erase( Iterator it )
{
  TAC_ASSERT( it.mNode != &mDummy );
  Node* next { it.mNode->mNext };
  EraseNode( it.mNode );
  return { next };
}

template< typename T >
void       Tac::List<T>::clear()
{
  Node* node { mDummy.mNext };
  while( node != &mDummy )
  {
    Node* next { node->mNext };
    TAC_DELETE node;
    node = next;
  }

  mDummy.mNext = &mDummy;
  mDummy.mPrev = &mDummy;
}


template< typename T >
void       Tac::List<T>::InsertBetween( Node* head, Node* tail, T t )
{
  Node* node = TAC_NEW Node
  {
    .mT { t },
    .mPrev { head },
    .mNext { tail },
  };
  head->mNext = node;
  tail->mPrev = node;
  mSize++;
}

template< typename T >
void Tac::List<T>::EraseNode( Node* node )
{
  TAC_ASSERT( node != &mDummy );
  node->mNext->mPrev = node->mPrev;
  node->mPrev->mNext = node->mNext;
  TAC_DELETE node;
  mSize--;
}


