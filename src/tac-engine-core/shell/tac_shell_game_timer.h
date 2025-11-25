#pragma once

#include "tac-std-lib/tac_ints.h"
#include "tac-engine-core/shell/tac_shell_game_time.h"
#include "tac-engine-core/shell/tac_shell_real_time.h"

namespace Tac
{
  struct GameTimer
  {
    static bool Update();
    static auto GetElapsedTime() -> GameTime;
    static auto GetElapsedFrames() -> GameFrame;
    static auto GetAccumulatedTime() -> TimeDelta;
    static auto GetLastTick() -> RealTime;
  };

  extern const int       TAC_FPS;
  extern const TimeDelta TAC_DT;
} // namespace Tac

