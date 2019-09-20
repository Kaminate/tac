#include "tacJobQueue.h"
#include "common/math/tacMath.h"

static void TacWorkerThread( TacJobQueue* jobQueue )
{
  while( jobQueue->mRunning )
  {
    TacJob* job = nullptr;
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
    job->SetStatus( TacAsyncLoadStatus::ThreadRunning );
    job->Execute();
    job->SetStatus( job->mErrors.empty() ?
      TacAsyncLoadStatus::ThreadCompleted :
      TacAsyncLoadStatus::ThreadFailed );
  }
}

TacJob::TacJob()
{
  mAsyncLoadStatus = TacAsyncLoadStatus::JustBeenCreated;
}
void TacJob::SetStatus( TacAsyncLoadStatus asyncLoadStatus )
{
  mStatusMutex.lock();
  mAsyncLoadStatus = asyncLoadStatus;
  mStatusMutex.unlock();
}

TacAsyncLoadStatus TacJob::GetStatus()
{
  mStatusMutex.lock();
  TacAsyncLoadStatus asyncLoadStatus = mAsyncLoadStatus;
  mStatusMutex.unlock();
  return asyncLoadStatus;
}

TacJobQueue* TacJobQueue::Instance = nullptr;
TacJobQueue::TacJobQueue()
{
  Instance = this;
}

void TacJobQueue::Push( TacJob* job )
{
  job->SetStatus( TacAsyncLoadStatus::ThreadQueued );
  job->mErrors.clear();
  mMutex.lock();
  mUnstarted.Push( job );
  mMutex.unlock();
}

void TacJobQueue::Init()
{
  int threadCount = TacMax( ( int )std::thread::hardware_concurrency(), mMinThreadCount );
  mThreads.resize( threadCount );
  mRunning = true;
  for( int i = 0; i < threadCount; ++i )
  {
    std::thread& curThread = mThreads[ i ];
    curThread = std::thread( TacWorkerThread, this );
    curThread.detach();
  }
}

