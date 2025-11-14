#include "tac_shell_timestep.h" // self-inc

#include "tac-engine-core/shell/tac_shell_timer.h"
#include "tac-std-lib/math/tac_math.h" // Max

// Test out different time steps:

const int               Tac::TAC_FPS = 20;
const Tac::TimeDuration Tac::TAC_DT{ .mSeconds { 1.0f / Tac::TAC_FPS } };

namespace Tac
{

  static Timer        sTimer         {};
  static Timestamp    sElapsedTime   {};
  static TimeDuration sAccumulator   {};
  static FrameIndex   sElapsedFrames {};

  bool Timestep::Update()
  {
    if( !sTimer.IsRunning() )
    {
      sTimer.Start();
      return true;
    }

    const TimeDuration dt{ sTimer.Tick() };
    sAccumulator.mSeconds = Fmod( sAccumulator.mSeconds + dt.mSeconds,
                                  TAC_DT.mSeconds * 2.0f );

    if( sAccumulator < TAC_DT )
      return false;


    sAccumulator -= TAC_DT;
    sElapsedTime += TAC_DT;
    sElapsedFrames++;
    return true;
  }
  auto Timestep::GetElapsedTime() -> Timestamp        { return sElapsedTime; }
  auto Timestep::GetElapsedFrames() -> FrameIndex     { return sElapsedFrames; }
  auto Timestep::GetAccumulatedTime() -> TimeDuration { return sAccumulator; }
  auto Timestep::GetLastTick() -> Timepoint           { return sTimer.GetLastTick(); }

} // namespace Tac
