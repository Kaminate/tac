#include "tac_shell_game_timer.h" // self-inc

#include "tac-engine-core/shell/tac_shell_real_timer.h"
#include "tac-std-lib/math/tac_math.h"

// Test out different time steps:

const int            Tac::TAC_FPS = 20;
const Tac::TimeDelta Tac::TAC_DT{ .mSeconds { 1.0f / Tac::TAC_FPS } };

namespace Tac
{
  static RealTimer sTimer         {};
  static GameTime  sElapsedTime   {};
  static TimeDelta sAccumulator   {};
  static GameFrame sElapsedFrames {};

  bool GameTimer::Update()
  {
    if( !sTimer.IsRunning() )
    {
      sTimer.Start();
      return true;
    }

    const TimeDelta dt{ sTimer.Tick() };
    sAccumulator.mSeconds = Fmod( sAccumulator.mSeconds + dt.mSeconds, TAC_DT.mSeconds * 2.0f );
    if( sAccumulator < TAC_DT )
      return false;

    sAccumulator -= TAC_DT;
    sElapsedTime += TAC_DT;
    sElapsedFrames++;
    return true;
  }
  auto GameTimer::GetElapsedTime() -> GameTime        { return sElapsedTime; }
  auto GameTimer::GetElapsedFrames() -> GameFrame     { return sElapsedFrames; }
  auto GameTimer::GetAccumulatedTime() -> TimeDelta   { return sAccumulator; }
  auto GameTimer::GetLastTick() -> RealTime           { return sTimer.GetLastTick(); }

} // namespace Tac
