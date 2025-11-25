#include "tac_shell_real_timer.h" // self-inc

#include "tac-std-lib/error/tac_assert.h"

namespace Tac
{
  void RealTimer::Start()
  {
    mStarted = true;
    mLastTick = RealTime::Now();
  }

  auto RealTimer::Tick() -> TimeDelta
  {
    TAC_ASSERT( mStarted );
    const RealTime now { RealTime::Now() };
    const TimeDelta seconds { now - mLastTick };
    mLastTick = now;
    return seconds;
  }

  bool RealTimer::IsRunning() const                { return mStarted; }

  auto RealTimer::GetLastTick() const -> RealTime { return mLastTick; }

} // namespace Tac
