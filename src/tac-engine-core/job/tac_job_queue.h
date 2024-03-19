#pragma once

#include "tac-std-lib/error/tac_error_handling.h"

import std; // atomic
//#include <atomic>

namespace Tac
{
  enum class JobState
  {
    JustBeenCreated,

    // Waiting for a worker thread to start the job
    ThreadQueued,

    // Set by the worker thread when it starts running
    ThreadRunning,

    // May be completed successfully, failed, or aborted
    ThreadFinished,
  };

  struct Job
  {
    Job();
    virtual               ~Job() = default;
    virtual void          Execute() = 0;
    void                  Abort();
    void                  SetState( JobState );
    bool                  AbortRequested() const;
    JobState              GetStatus() const;

    //                    Errors which occurred while running the job in another thread.
    Errors                mErrors;
    std::atomic<JobState> mAsyncLoadStatus;
    std::atomic<bool>     mAbortRequested;
  };

  void JobQueueInit();
  void JobQueuePush( Job* );


} // namespace Tac

