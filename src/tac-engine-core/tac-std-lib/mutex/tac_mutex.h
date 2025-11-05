#pragma once

#include "tac-std-lib/preprocess/tac_preprocessor.h"

namespace Tac
{
  struct Mutex
  {
    Mutex();
    Mutex( const Mutex& ) = delete;
    Mutex( Mutex&& ) = delete;
    ~Mutex();
    void operator = ( const Mutex& ) = delete;
    void operator = ( Mutex&& ) = delete;
    void lock();
    void unlock();

  private:
    void* mImpl;
  };

  struct LockGuard
  {
    LockGuard( Mutex& );
    ~LockGuard();
    Mutex& mMutex;
  };
}
