
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

// nahh span is garbo

//#pragma once
//
//namespace Tac
//{
//  template< typename T >
//  struct Span
//  {
//    Span() = default;
//    Span( T* elements, int count ) : mElements( elements ), mCount( count ) {}
//    int    size() { return mCount; }
//    T*     begin() { return mElements; };
//    T*     end() { return mElements + mCount; }
//    T*     data() { return mElements; }
//    T&     operator[]( int index ) { return mElements[ index ]; }
//
//    T*     mElements = nullptr;
//    int    mCount = 0;
//  };
//
//  typedef Span< char > MemorySpan;
//
//  // cant do this or else the operator[] returns a void&
//  //typedef Span< void > MemorySpan;
//
//
//}
//
