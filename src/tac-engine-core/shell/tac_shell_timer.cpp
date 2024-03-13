#include "tac_shell_timer.h" // self-inc

#include "tac-std-lib/error/tac_assert.h"

namespace Tac
{

  Timepoint Timepoint::Now()
  {
    return Timepoint{ clock::now() };
  }

  void Timepoint::operator -= ( TimestampDifference d )
  {
    const double nsDouble = d.mSeconds * 1e9;
    const nanoseconds::rep nsRep = ( nanoseconds::rep )nsDouble;
    const nanoseconds ns( nsRep );
    mTimePoint -= ns;
  }

  TimestampDifference operator - ( const Timepoint& a, const Timepoint& b )
  {
    const Timepoint::nanoseconds c = a.mTimePoint - b.mTimePoint;
    const Timepoint::nanoseconds::rep n = c.count();
    const double seconds = n / 1e9;
    return TimestampDifference{ ( float )seconds };
  }

  void                Timer::Start()
  {
    mStarted = true;
    mLastTick = Timepoint::Now();
  }

  TimestampDifference Timer::Tick()
  {
    TAC_ASSERT( mStarted );

    const Timepoint now = Timepoint::Now();
    const TimestampDifference seconds = now - mLastTick;
    mLastTick = now;
    return ( float )seconds;
  }

  bool                Timer::IsRunning() const   { return mStarted; }
  Timepoint           Timer::GetLastTick() const { return mLastTick; }



} // namespace Tac
