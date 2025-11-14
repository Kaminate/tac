#include "tac_shell_timer.h" // self-inc

#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <chrono>
#endif

namespace Tac
{

  static_assert( is_same< std::chrono::nanoseconds::rep, Timepoint::NanosecondDuration >::value );

  // -----------------------------------------------------------------------------------------------

  Timepoint::Timepoint( NanosecondDuration ns )                   { mTimeSinceEpoch = ns; }

  auto Timepoint::Now() -> Timepoint
  {
    //       std::time_point                           std::duration      long long
    return { std::chrono::high_resolution_clock::now().time_since_epoch().count() };
  }

  void Timepoint::operator -= ( TimeDuration d )
  {
    mTimeSinceEpoch -= ( Timepoint::NanosecondDuration )( d.mSeconds * 1e9 );
  }

  auto Timepoint::TimeSinceEpoch() const -> NanosecondDuration    { return mTimeSinceEpoch; }

  auto operator - ( const Timepoint& a, const Timepoint& b ) -> TimeDuration
  {
    const Timepoint::NanosecondDuration ns{ a.TimeSinceEpoch() - b.TimeSinceEpoch() };
    const double seconds{ ns / 1e9 };
    return TimeDuration{ ( float )seconds };
  }

  // -----------------------------------------------------------------------------------------------

  void Timer::Start()
  {
    mStarted = true;
    mLastTick = Timepoint::Now();
  }

  auto Timer::Tick() -> TimeDuration
  {
    TAC_ASSERT( mStarted );
    const Timepoint now { Timepoint::Now() };
    const TimeDuration seconds { now - mLastTick };
    mLastTick = now;
    return seconds;
  }

  bool Timer::IsRunning() const                { return mStarted; }

  auto Timer::GetLastTick() const -> Timepoint { return mLastTick; }

} // namespace Tac
