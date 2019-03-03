#pragma once
#include <initializer_list>  // std::initializer_list
#include <utility> // std::move

template< typename T >
struct TacVector
{
  TacVector() = default;
  template< typename Iterator >
  TacVector( Iterator iBegin, Iterator iEnd )
  {
    while( iBegin != iEnd )
    {
      T t = *iBegin++;
      push_back( t );
    }
  }
  TacVector( const TacVector& v )
  {
    int size = v.size();
    resize( size );
    for( int i = 0; i < size; ++i )
      mTs[ i ] = v[ i ];
  }
  TacVector( int size )
  {
    resize( size );
  }
  TacVector( int size, T initialValue )
  {
    resize( size );
    for( int i = 0; i < size; ++i )
      mTs[ i ] = initialValue;
  }
  TacVector( T* tbegin, T* tend )
  {
    int size = ( int )( tend - tbegin );
    resize( size );
    for( int i = 0; i < size; ++i )
      mTs[ i ] = tbegin[ i ];
  }
  TacVector( std::initializer_list< T > ts )
  {
    resize( ( int )ts.size() );
    int i = 0;
    for( T t : ts )
      mTs[ i++ ] = t;
  }
  ~TacVector()
  {
    delete[] mTs;
  }
  void operator =( const TacVector<T>& v )
  {
    int newSize = v.size();
    resize( newSize );
    for( int i = 0; i < newSize; ++i )
      mTs[ i ] = v[ i ];
  }
  void clear()
  {
    delete[] mTs;
    mTs = nullptr;
    mTCount = 0;
    mTCapacity = 0;
  }
  int size() const
  {
    return mTCount;
  }
  void push_back( T t )
  {
    int newSize = mTCount + 1;
    if( newSize > mTCapacity )
    {
      int newCapacity = int( mTCount * 1.5f );
      if( newCapacity < newSize )
        newCapacity = newSize;
      reserve( newCapacity );
    }
    mTs[ mTCount++ ] = t;
  }
  void pop_back()
  {
    mTCount--;
  }
  bool empty() const { return !mTCount; }
  void resize( int newSize )
  {
    if( newSize > mTCount )
      reserve( newSize );
    mTCount = newSize;
  }
  void reserve( int capacity )
  {
    if( capacity < mTCapacity )
      return;
    T* newTs = new T[ capacity ];
    for( int i = 0; i < mTCount; ++i )
    {
      // std::move for TacVector< std::thread >
      newTs[ i ] = std::move( mTs[ i ] );
    }
    delete[] mTs;
    mTs = newTs;
    mTCapacity = capacity;
  }
  T* begin() { return mTs; };
  T* end() { return mTs + mTCount; };
  const T* begin() const { return mTs; };
  const T* end() const { return mTs + mTCount; };
  T& front() { return *begin(); }
  T& back() { return mTs[ mTCount - 1 ]; }
  T* data() { return mTs; }
  const T* data() const { return mTs; }
  T& operator[]( int i ) { return mTs[ i ]; }
  const T& operator[]( int i ) const { return mTs[ i ]; }
  T* mTs = nullptr;
  int mTCount = 0;
  int mTCapacity = 0;
};
