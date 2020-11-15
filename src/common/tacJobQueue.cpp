
#include "src/common/tacJobQueue.h"
#include "src/common/math/tacMath.h"

namespace Tac
{
  const int                    kMinThreadCount = 4;
  static bool                  sJobQueueRunning = false;
  static Vector< std::thread > sJobQueueThreads;
  static std::mutex            sJobQueueMutex;

  // The jobs are unowned
  static RingVector< Job* >    sJobQueueUnstarted;
  static float                 sJobQueueMinJobSeconds = 0;

  int JobQueueGetThreadCount()
  {
    return sJobQueueThreads.size();
  }


  static void WorkerThread()
  {
    while( sJobQueueRunning )
    {
      Job* job = nullptr;
      sJobQueueMutex.lock();
      if( !sJobQueueUnstarted.empty() )
        job = sJobQueueUnstarted.Pop();
      sJobQueueMutex.unlock();
      // why?
      if( !job )
      {
        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        continue;
      }
      std::this_thread::sleep_for( std::chrono::milliseconds( ( int )( sJobQueueMinJobSeconds * 1000 ) ) );
      job->SetStatus( AsyncLoadStatus::ThreadRunning );
      job->Execute();
      job->SetStatus( job->mErrors.empty() ?
                      AsyncLoadStatus::ThreadCompleted :
                      AsyncLoadStatus::ThreadFailed );
    }
  }

  Job::Job()
  {
    mAsyncLoadStatus = AsyncLoadStatus::JustBeenCreated;
  }

  void Job::SetStatus( AsyncLoadStatus asyncLoadStatus )
  {
    mStatusMutex.lock();
    mAsyncLoadStatus = asyncLoadStatus;
    mStatusMutex.unlock();
  }

  AsyncLoadStatus Job::GetStatus()
  {
    mStatusMutex.lock();
    AsyncLoadStatus asyncLoadStatus = mAsyncLoadStatus;
    mStatusMutex.unlock();
    return asyncLoadStatus;
  }

  void JobQueuePush( Job* job )
  {
    job->SetStatus( AsyncLoadStatus::ThreadQueued );
    job->mErrors.clear();
    sJobQueueMutex.lock();
    sJobQueueUnstarted.Push( job );
    sJobQueueMutex.unlock();
  }

  void JobQueueInit()
  {
    const int threadCount = Max( ( int )std::thread::hardware_concurrency(), kMinThreadCount );
    sJobQueueThreads.resize( threadCount );
    sJobQueueRunning = true;
    for( int i = 0; i < threadCount; ++i )
    {
      std::thread& curThread = sJobQueueThreads[ i ];
      curThread = std::thread( WorkerThread );
      curThread.detach();
    }
  }
}

