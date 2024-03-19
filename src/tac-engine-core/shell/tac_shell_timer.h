#pragma once

#include "tac-engine-core/shell/tac_shell_timestamp.h"
#include "tac-std-lib/tac_ints.h"

//import std; // chrono

namespace Tac
{
  struct Timepoint
  {
    using NanosecondDuration = i64;

    Timepoint() = default;
    Timepoint( NanosecondDuration );
    NanosecondDuration TimeSinceEpoch() const;
    void operator -= ( TimestampDifference );

    static Timepoint Now();

  private:
    NanosecondDuration mTimeSinceEpoch = 0;
  };

  TimestampDifference operator - (const Timepoint&, const Timepoint&);
  
  struct Timer
  {

    void                Start();
    TimestampDifference Tick();
    bool                IsRunning() const;
    Timepoint           GetLastTick() const;

  private:
    Timepoint           mLastTick{};
    bool                mStarted = false;
  };

} // namespace Tac

