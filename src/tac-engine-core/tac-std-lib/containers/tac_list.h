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
    operator bool()        { return mNode; }
    bool operator == ( const ConstIterator& ) const = default;
    auto operator *() -> const T&  { return mNode->mT; }
    auto operator ->() -> dynmc T* { return &mNode->mT; }
    void operator ++() { mNode = mNode->mNext; }
    void operator --() { mNode = mNode->mPrev; } // pre-dec
    auto operator --(int) -> ConstIterator // post-dec
    {
      ConstIterator it { *this };
      mNode = mNode->mPrev;
      return it;
    } 

    const Node*   mNode {};
  };

  struct Iterator
  {
    operator bool()        { return mNode; }
    bool operator == ( const Iterator& ) const = default;
    auto operator *() -> dynmc T& { return mNode->mT; }
    auto operator ->() -> dynmc T* { return &mNode->mT; }
    void operator ++() { mNode = mNode->mNext; }
    void operator --() { mNode = mNode->mPrev; } // pre-dec
    auto operator --(int) -> Iterator // post-dec
    {
      Iterator it { *this };
      mNode = mNode->mPrev;
      return it;
    } 

    dynmc Node* mNode {};
  };

  List() = default;
  List(const List& other) : List()          { assign( other ); }
  ~List()                                   { clear(); }


  void push_front( T t )                    { InsertAfter( &mDummy, t ); }
  void push_back( T t )                     { InsertBefore( &mDummy, t ); }
  auto pop_front() -> T;
  auto pop_back() -> T;
  auto front() dynmc -> dynmc T&            { return mDummy.mNext->mT; }
  auto front() const -> const T&            { return mDummy.mNext->mT; }
  auto back() dynmc -> dynmc T&             { return mDummy.mPrev->mT; }
  auto back() const -> const T&             { return mDummy.mPrev->mT; }
  auto begin() dynmc -> Iterator            { return { mDummy.mNext }; }
  auto begin() const -> ConstIterator       { return { mDummy.mNext }; }
  auto rbegin() dynmc -> Iterator           { return { mDummy.mPrev }; }
  auto rbegin() const -> ConstIterator      { return { mDummy.mPrev }; }
  auto end() dynmc -> Iterator              { return { &mDummy }; }
  auto end() const -> ConstIterator         { return { &mDummy }; }
  auto Find( T  t ) dynmc -> Iterator       { return { FindNode( t ) }; }
  auto Find( T t ) const -> ConstIterator   { return { FindNode( t ) }; }
  auto empty() const -> bool                { return !mSize; }
  auto size() const -> int                  { return mSize; }
  void clear();
  auto erase( Iterator it ) -> Iterator; // returns the next iterator after `it`
  void operator = ( const List& l ) { assign( l ); }

private:

  void assign( const List& );
  auto FindNode( T ) -> Node*;
  void EraseNode( Node* );
  void InsertAfter( Node* n, T t )  { InsertBetween( n, n->mNext, t ); }
  void InsertBefore( Node* n, T t ) { InsertBetween( n->mPrev, n, t ); }
  void InsertBetween( Node*, Node*, T );

  Node          mDummy{ .mPrev = &mDummy, .mNext = &mDummy };
  int           mSize {  };
};


template< typename T >
void Tac::List<T>::assign( const List& other )
{
  clear();
  for( const T& t : other )
    push_back( t );
}

template< typename T >
auto Tac::List<T>::pop_front() -> T
{
  TAC_ASSERT( !empty() );
  const T t { mDummy.mNext->mT };
  EraseNode( mDummy.mNext );
  return t;
}

template< typename T >
auto Tac::List<T>::pop_back() -> T
{
  TAC_ASSERT( !empty() );
  T t { mDummy.mPrev.mT };
  EraseNode( mDummy.mPrev );
  return t;
}

template< typename T >
auto Tac::List<T>::FindNode( T t ) -> Node* 
{
  for( Node* node { mDummy.mNext }; node != &mDummy; node = node->mNext )
    if( node->mT == t )
      return node;
  return nullptr;
}

template< typename T >
auto Tac::List<T>::erase( Iterator it ) -> Iterator
{
  TAC_ASSERT( it.mNode != &mDummy );
  Node* next { it.mNode->mNext };
  EraseNode( it.mNode );
  return { next };
}

template< typename T >
void Tac::List<T>::clear()
{
  Node* node { mDummy.mNext };
  while( node != &mDummy )
  {
    Node* next{ node->mNext };
    TAC_DELETE node;
    node = next;
  }

  mSize = 0;
  mDummy.mNext = &mDummy;
  mDummy.mPrev = &mDummy;
}


template< typename T >
void Tac::List<T>::InsertBetween( Node* head, Node* tail, T t )
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


