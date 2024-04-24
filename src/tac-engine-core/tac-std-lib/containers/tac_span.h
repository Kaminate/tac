#pragma once

#include "tac-std-lib/error/tac_assert.h"

namespace Tac
{


  // Span is basically a StringView for an Array/Vector/etc
  // So it should be used to pass data to functions,
  // but not to store the data
  template< typename T >
  struct Span
  {
    Span( T* ts, int tCount )  : mTs{ ts }, mTCount{ tCount } {}
    Span( T* t )               : mTs{ t }, mTCount{ 1 } {}
    Span( T* tBegin, T* tEnd ) : mTs{ tBegin }, mTCount{ int( tEnd - tBegin ) } {}
    Span( T& t )               : mTs{ &t }, mTCount{ 1 } {}
    Span( T&& t ) = delete;

    int      size() const              { return mTCount; }
    bool     empty() const             { return !mTCount; }
    T&       operator[]( int i )       { return mTs[ i ]; }
    const T& operator[]( int i ) const { return mTs[ i ]; }
    const T* data() const              { return mTs; }
    T*       data()                    { return mTs; }
    T*       begin()                   { return mTs; }
    const T* begin() const             { return mTs; }
    T*       end()                     { return mTs + mTCount; }
    const T* end() const               { return mTs + mTCount; }

    void operator = ( const Span<T> other )
    {
      TAC_ASSERT( mTCount == other.mTCount );
      for( int i{}; i < mTCount; ++i )
        mTs[ i ] = other.mTs[ i ];
    }

  private:
    T* mTs;
    int mTCount;
  };

} // namespace Tac
