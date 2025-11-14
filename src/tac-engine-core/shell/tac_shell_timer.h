#pragma once

#include "tac-engine-core/shell/tac_shell_timestamp.h"
#include "tac-std-lib/tac_ints.h"

namespace Tac
{
  struct Timepoint
  {
    using NanosecondDuration = i64;

    Timepoint() = default;
    Timepoint( NanosecondDuration );
    auto TimeSinceEpoch() const -> NanosecondDuration;
    void operator -= ( TimeDuration );

    static auto Now() -> Timepoint;

  private:
    NanosecondDuration mTimeSinceEpoch {};
  };

  auto operator - ( const Timepoint&, const Timepoint& ) -> TimeDuration;

  struct Timer
  {
    void Start();
    auto Tick() -> TimeDuration;
    bool IsRunning() const;
    auto GetLastTick() const -> Timepoint;

  private:
    Timepoint mLastTick {};
    bool      mStarted  {};
  };

} // namespace Tac

