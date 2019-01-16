// The whole point of a span is to unify the way we pass around contiguous data,
// as a convenience class. Normally you would have something like these:
//
//   void Foo( int* elements, int* elementCount )
//   void Foo( int* elementBegin, int* elementEnd )
//
// But with a span we just don't worry about it anymore,
// and everything becomes:
//
//   void Foo( span< int > elements )
//

#pragma once

template< typename T >
struct TacSpan
{
  int size(){ return mCount; }
  T* data(){ return mElements; }
  T& operator[]( int index ){ return mElements[ index ]; }

  T* mElements = nullptr;
  int mCount = 0;
};

typedef TacSpan< char > TacMemorySpan;

// cant do this or else the operator[] returns a void&
//typedef TacSpan< void > TacMemorySpan;

