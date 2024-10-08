#pragma once

#include "tac-std-lib/tac_ints.h"
#include "tac-engine-core/shell/tac_shell_timestamp.h"
#include "tac-engine-core/shell/tac_shell_timer.h"

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

  extern const int                 TAC_FRAMES_PER_SECOND;
  extern const TimestampDifference TAC_DELTA_FRAME_SECONDS;


} // namespace Tac

