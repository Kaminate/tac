#pragma once

//#include "tac-std-lib/preprocess/tac_preprocessor.h" // TAC_ASSERT
#include "tac-std-lib/error/tac_assert.h"

namespace Tac
{
  template < typename T >
  struct Optional
  {
    Optional() = default;
    Optional( T t )                     : mT( t ), mExist( true ) {}
    // this should maybe return a constref
    T    GetValue() const               { TAC_ASSERT( mExist ); return mT; }
    T    GetValueOr( T t ) const        { return mExist ? mT : t; }
    bool HasValue() const               { return mExist; }
    operator bool() const               { return mExist; }
    T        operator *() const         { return mT; }
    const T* operator ->() const        { return &mT; }
    T*       operator ->()              { return &mT; }
    bool     operator == ( const T& t ) { return mExist && mT == t; }
    bool     operator != ( const T& t ) { return !mExist || mT != t; }
    void     operator = ( const T& t )  { mT = t; mExist = true; }
  private:
    T    mT{};
    bool mExist{};
  };
}

