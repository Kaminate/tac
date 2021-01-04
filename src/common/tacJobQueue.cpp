#include "src/common/containers/tacRingVector.h"
#include "src/common/containers/tacVector.h"
#include "src/common/tacJobQueue.h"
#include "src/common/math/tacMath.h"

#include <thread>

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
    const std::thread::id id = std::this_thread::get_id();
    while( sJobQueueRunning )
    {
      std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
      Job* job = nullptr;
      sJobQueueMutex.lock();
      if( !sJobQueueUnstarted.empty() )
        job = sJobQueueUnstarted.Pop();
      sJobQueueMutex.unlock();
      if( !job )
        continue;
      std::this_thread::sleep_for( std::chrono::milliseconds( ( int )( sJobQueueMinJobSeconds * 1000 ) ) );
      job->SetState( JobState::ThreadRunning );
      job->Execute();
      job->SetState( JobState::ThreadFinished );
    }
  }

  void Job::Abort()
  {
    mStatusMutex.lock();
    mErrors.Append( "Job aborted by user" );
    mStatusMutex.unlock();
  }

  bool Job::AbortRequested() const
  {
    return mErrors;
  }

  void Job::SetState( JobState JobState )
  {
    mStatusMutex.lock();
    mAsyncLoadStatus = JobState;
    mStatusMutex.unlock();
  }

  JobState Job::GetStatus() const
  {
    mStatusMutex.lock();
    const JobState JobState = mAsyncLoadStatus;
    mStatusMutex.unlock();
    return JobState;
  }

  void JobQueuePush( Job* job )
  {
    job->SetState( JobState::ThreadQueued );
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

