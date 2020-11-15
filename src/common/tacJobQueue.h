
#pragma once


#include "src/common/containers/tacRingVector.h"
#include "src/common/containers/tacVector.h"
#include "src/common/tacErrorHandling.h"

#include <mutex>
#include <thread>

namespace Tac
{
  enum class AsyncLoadStatus
  {
    JustBeenCreated,

    // Waiting for a worker thread to start the job
    ThreadQueued,

    // Set by the worker thread when it starts running
    ThreadRunning,

    // Set by the worker thread when something fails ( stores error message in the thread data )
    ThreadFailed,

    // Set by the worker thread when it's done
    ThreadCompleted
  };


  struct Job
  {
    Job();
    virtual               ~Job() = default;
    virtual void          Execute() = 0;
    void                  SetStatus( AsyncLoadStatus );
    AsyncLoadStatus       GetStatus();

    // Errors which occured while running the job in another thread.
    Errors                mErrors;
  private:
    AsyncLoadStatus       mAsyncLoadStatus;
    std::mutex            mStatusMutex;
  };

  void JobQueueInit();
  void JobQueuePush( Job* );


}

