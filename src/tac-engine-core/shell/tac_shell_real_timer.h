#pragma once

#include "tac-engine-core/shell/tac_shell_real_time.h"
#include "tac-std-lib/tac_ints.h"

namespace Tac
{
  struct RealTimer
  {
    void Start();
    auto Tick() -> TimeDelta;
    bool IsRunning() const;
    auto GetLastTick() const -> RealTime;

  private:
    RealTime mLastTick {};
    bool     mStarted  {};
  };

} // namespace Tac

