#pragma once

#include "tac-std-lib/tac_ints.h"
#include "tac-engine-core/shell/tac_shell_timestamp.h"
#include "tac-engine-core/shell/tac_shell_timer.h"

namespace Tac
{
  using FrameIndex = u64;

  struct Timestep
  {
    static bool Update();
    static auto GetElapsedTime() -> Timestamp;
    static auto GetElapsedFrames() -> FrameIndex;
    static auto GetAccumulatedTime() -> TimeDuration;
    static auto GetLastTick() -> Timepoint;
  };

  extern const int          TAC_FPS;
  extern const TimeDuration TAC_DT;
} // namespace Tac

