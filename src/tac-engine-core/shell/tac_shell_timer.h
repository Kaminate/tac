#pragma once

#include "tac-engine-core/shell/tac_shell_timestamp.h"

import std; // chrono

namespace Tac
{
  struct Timepoint
  {
    using clock       = std::chrono::high_resolution_clock;
    using nanoseconds = std::chrono::nanoseconds;
    using time_point  = std::chrono::time_point< clock, nanoseconds >;

    static Timepoint Now();
    void operator -= ( TimestampDifference );

    time_point mTimePoint;
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

