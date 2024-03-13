#pragma once

#include "tac-std-lib/tac_ints.h"
#include "tac-std-lib/shell/tac_shell_timestamp.h"
#include "tac-std-lib/shell/tac_shell_timer.h"

namespace Tac
{
  using FrameIndex = u64;

  struct Timestep
  {
    static bool                Update();
    static Timestamp           GetElapsedTime();
    static FrameIndex          GetElapsedFrames();
    static TimestampDifference GetAccumulatedTime();
    static Timepoint           GetLastTick();
  };

  // Fun to change to 1 for debugging
  const int                 TAC_FRAMES_PER_SECOND = 60;
  const TimestampDifference TAC_DELTA_FRAME_SECONDS = 1.0f / TAC_FRAMES_PER_SECOND;


} // namespace Tac

