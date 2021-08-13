#pragma once

#include <initializer_list>

namespace Tac
{
  template< typename T, int N >
  struct FixedVector
  {
    FixedVector() = default;
    FixedVector( T t )      { push_back( t ); }
    FixedVector( int size ) { resize( size ); }
    FixedVector( int size, T initialValue )
    {
      resize( size );
      for( int i = 0; i < size; ++i )
        mTs[ i ] = initialValue;
    }
    FixedVector( T* tbegin, T* tend )
    {
      int size = ( int )( tend - tbegin );
      resize( size );
      for( int i = 0; i < size; ++i )
        mTs[ i ] = tbegin[ i ];
    }
    FixedVector( std::initializer_list< T > ts ) { for( const T& t : ts ) push_back( t ); }
    void     clear()                   { mTCount = 0; }
    int      size() const              { return mTCount; }
    bool     empty() const             { return !mTCount; }
    void     resize( int newSize )     { mTCount = newSize; }
    void     push_back( const T& t )   { mTs[ mTCount++ ] = t; }
    void     pop_back()                { TAC_ASSERT( mTCount ); mTCount--; }
    T*       begin()                   { return mTs; };
    const T* begin() const             { return mTs; };
    T*       end()                     { return mTs + mTCount; };
    const T* end() const               { return mTs + mTCount; };
    T&       front()                   { return *begin(); }
    T&       back()                    { return mTs[ mTCount - 1 ]; }
    T*       data()                    { return mTs; }
    const T* data() const              { return mTs; }
    T&       operator[]( int i )       { return mTs[ i ]; }
    const T& operator[]( int i ) const { return mTs[ i ]; }
    T        mTs[ N ];
    int      mTCount = 0;
  };
}
