#pragma once

#include "src/common/tacErrorHandling.h"

#include <mutex>

namespace Tac
{
  enum class JobState
  {
    JustBeenCreated,

    // Waiting for a worker thread to start the job
    ThreadQueued,

    // Set by the worker thread when it starts running
    ThreadRunning,

    // May be completed successfuly, failed, or aborted
    ThreadFinished,
  };


  struct Job
  {
    virtual               ~Job() = default;
    virtual void          Execute() = 0;
    void                  Abort();
    void                  SetState( JobState );
    bool                  AbortRequested() const;
    JobState              GetStatus() const;

    // Errors which occured while running the job in another thread.
    Errors                mErrors;
    JobState              mAsyncLoadStatus = JobState::JustBeenCreated;
    mutable std::mutex    mStatusMutex;
  };

  void JobQueueInit();
  void JobQueuePush( Job* );


}

