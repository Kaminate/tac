
#include "src/common/tacJobQueue.h"
#include "src/common/math/tacMath.h"

namespace Tac
{
static void WorkerThread( JobQueue* jobQueue )
{
  while( jobQueue->mRunning )
  {
    Job* job = nullptr;
    jobQueue->mMutex.lock();
    if( !jobQueue->mUnstarted.empty() )
      job = jobQueue->mUnstarted.Pop();
    jobQueue->mMutex.unlock();
    if( !job )
    {
      std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
      continue;
    }
    //std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
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

JobQueue* JobQueue::Instance = nullptr;
JobQueue::JobQueue()
{
  Instance = this;
}

void JobQueue::Push( Job* job )
{
  job->SetStatus( AsyncLoadStatus::ThreadQueued );
  job->mErrors.clear();
  mMutex.lock();
  mUnstarted.Push( job );
  mMutex.unlock();
}

void JobQueue::Init()
{
  int threadCount = Max( ( int )std::thread::hardware_concurrency(), mMinThreadCount );
  mThreads.resize( threadCount );
  mRunning = true;
  for( int i = 0; i < threadCount; ++i )
  {
    std::thread& curThread = mThreads[ i ];
    curThread = std::thread( WorkerThread, this );
    curThread.detach();
  }
}
}

