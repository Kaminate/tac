#include "tac_job_queue.h" // self-inc

#include "tac-std-lib/containers/tac_ring_vector.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/math/tac_math.h"

#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <thread>
  #include <mutex>
#endif

namespace Tac
{
  const int                    kMinThreadCount        { 4 };
  static bool                  sJobQueueRunning       {};
  static Vector< std::thread > sJobQueueThreads       {};
  static std::mutex            sJobQueueMutex         {};

  // The jobs are unowned
  static RingVector< Job* >    sJobQueueUnstarted     {};
  static float                 sJobQueueMinJobSeconds {};

  //int JobQueueGetThreadCount()
  //{
  //  return sJobQueueThreads.size();
  //}

  static Job* GetJob()
  {
    TAC_SCOPE_GUARD( std::lock_guard, sJobQueueMutex );
    return sJobQueueUnstarted.empty() ? nullptr : sJobQueueUnstarted.Pop();
  }

  static void WorkerThread()
  {
    const std::chrono::milliseconds minJobRunMsec( ( int )( sJobQueueMinJobSeconds * 1000 ) );
    const std::chrono::milliseconds minJobGetMsec( 1 );

    // const std::thread::id id = std::this_thread::get_id();
    while( sJobQueueRunning )
    {
      std::this_thread::sleep_for( minJobGetMsec );

      if( Job * job{ GetJob() } )
      {
        std::this_thread::sleep_for( minJobRunMsec );

        job->SetState( JobState::ThreadRunning );
        job->Execute( job->mErrors );
        job->SetState( JobState::ThreadFinished );
      }
    }
  }

  Job::Job()
  {
    mAsyncLoadStatus = JobState::JustBeenCreated;
    mAbortRequested = false;
  }

  void Job::Abort()
  {
    //mStatusMutex.lock();
    //mErrors.Append( "Job aborted by user" );
    //mStatusMutex.unlock();
    mAbortRequested = true;
  }

  bool Job::AbortRequested() const
  {
    return mAbortRequested;
    //return mErrors;
  }

  void Job::SetState( JobState JobState )
  {
    //mStatusMutex.lock();
    mAsyncLoadStatus = JobState;
    //mStatusMutex.unlock();
  }

  void Job::Clear()
  {
    mErrors.clear();
    mAsyncLoadStatus = JobState::Cleared;
    mAbortRequested = false;
  }

  JobState Job::GetStatus() const
  {
    //mStatusMutex.lock();
    const JobState JobState{ mAsyncLoadStatus };
    //mStatusMutex.unlock();
    return JobState;
  }
}

void Tac::JobQueuePush( Job* job )
{
  job->SetState( JobState::ThreadQueued );
  job->mErrors.clear();
  sJobQueueMutex.lock();
  sJobQueueUnstarted.Push( job );
  sJobQueueMutex.unlock();
}

void Tac::JobQueueInit()
{
  const int threadCount{ Max( ( int )std::thread::hardware_concurrency(), kMinThreadCount ) };
  sJobQueueThreads.resize( threadCount );
  sJobQueueRunning = true;
  for( int i{}; i < threadCount; ++i )
  {
    std::thread& curThread{ sJobQueueThreads[ i ] };
    curThread = std::thread( WorkerThread );
    curThread.detach();
  }
}

