#include "tac_shell_timer.h" // self-inc

#include "tac-std-lib/error/tac_assert.h"

import std; // chrono

namespace Tac
{
  struct true_type { static const bool value = true; };
  struct false_type { static const bool value = true; };
  template< typename T, typename U > struct is_same : public false_type {};
  template< typename T >             struct is_same< T, T > : public true_type {};

  static_assert( is_same< std::chrono::nanoseconds::rep, Timepoint::NanosecondDuration >::value );

  // -----------------------------------------------------------------------------------------------

  Timepoint Timepoint::Now()
  {
    //       std::time_point                           std::duration      long long
    return { std::chrono::high_resolution_clock::now().time_since_epoch().count() };
  }

  void Timepoint::operator -= ( TimestampDifference d )
  {
    mTimeSinceEpoch -= ( Timepoint::NanosecondDuration )( d.mSeconds * 1e9 );
  }

  Timepoint::Timepoint( NanosecondDuration ns ) { mTimeSinceEpoch = ns; }
  Timepoint::NanosecondDuration Timepoint::TimeSinceEpoch() const { return mTimeSinceEpoch; }

  TimestampDifference operator - ( const Timepoint& a, const Timepoint& b )
  {
    const Timepoint::NanosecondDuration ns { a.TimeSinceEpoch() - b.TimeSinceEpoch() };
    const double seconds { ns / 1e9 };
    return TimestampDifference{ ( float )seconds };
  }

  // -----------------------------------------------------------------------------------------------

  void                Timer::Start()
  {
    mStarted = true;
    mLastTick = Timepoint::Now();
  }

  TimestampDifference Timer::Tick()
  {
    TAC_ASSERT( mStarted );

    const Timepoint now { Timepoint::Now() };
    const TimestampDifference seconds { now - mLastTick };
    mLastTick = now;
    return ( float )seconds;
  }

  bool                Timer::IsRunning() const   { return mStarted; }
  Timepoint           Timer::GetLastTick() const { return mLastTick; }

} // namespace Tac
