#pragma once


#include "common/containers/tacArray.h"
#include "common/containers/tacRingVector.h"
#include "common/containers/tacVector.h"
#include "common/tacErrorHandling.h"

#include <mutex>
#include <thread>

enum class TacAsyncLoadStatus
{
  // Waiting for a worker thread to start the job
  ThreadQueued,

  // Set by the worker thread when it starts running
  ThreadRunning,

  // Set by the worker thread when something fails ( stores error message in the thread data )
  ThreadFailed,

  // Set by the worker thread when it's done
  ThreadCompleted
};

struct TacJob
{
  virtual ~TacJob() = default;
  virtual void Execute() = 0;
  void SetStatus( TacAsyncLoadStatus asyncLoadStatus );
  TacAsyncLoadStatus GetStatus();

  // Errors which occured while running the job in another thread.
  TacErrors mErrors;
private:
  TacAsyncLoadStatus mAsyncLoadStatus = TacAsyncLoadStatus::ThreadQueued;
  std::mutex mStatusMutex;
};

struct TacJobQueue
{
  void Init();
  void Push( TacJob* job );
  int GetThreadCount() const { return mThreads.size(); }

  int mMinThreadCount = 4;
  bool mRunning = false;
  TacVector< std::thread > mThreads;
  std::mutex mMutex;

  // owned
  TacRingVector< TacJob* > mUnstarted;
};

