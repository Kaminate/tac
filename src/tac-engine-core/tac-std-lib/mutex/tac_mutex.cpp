#include "tac_mutex.h" // self-inc

#include "tac-std-lib/memory/tac_memory.h"

#if TAC_SHOULD_IMPORT_STD()
import std;
#else
#include <mutex>
#endif

namespace Tac
{
  Mutex::Mutex()
  {
    mImpl = TAC_NEW std::mutex();
  }

  Mutex::~Mutex()
  {
    TAC_DELETE( std::mutex* )mImpl;
  }

  void Mutex::lock()
  {
    ( ( std::mutex* )mImpl )->lock();
  }

  void Mutex::unlock()
  {
    ( ( std::mutex* )mImpl )->unlock();
  }

  LockGuard::LockGuard( Mutex& m ) : mMutex{ m }
  {
    mMutex.lock();
  }

  LockGuard::~LockGuard()
  {
    mMutex.unlock();
  }
}
