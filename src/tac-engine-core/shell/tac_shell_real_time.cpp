#include "tac_shell_real_time.h" // self-inc

#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <chrono>
#endif

namespace Tac
{

  static_assert( is_same< std::chrono::nanoseconds::rep, RealTime::NanosecondDuration >::value );

  // -----------------------------------------------------------------------------------------------

  RealTime::RealTime( NanosecondDuration ns )                   { mTimeSinceEpoch = ns; }

  auto RealTime::Now() -> RealTime
  {
    //       std::time_point                           std::duration      long long
    return { std::chrono::high_resolution_clock::now().time_since_epoch().count() };
  }

  void RealTime::operator -= ( TimeDelta d )
  {
    mTimeSinceEpoch -= ( RealTime::NanosecondDuration )( d.mSeconds * 1e9 );
  }

  auto RealTime::TimeSinceEpoch() const -> NanosecondDuration    { return mTimeSinceEpoch; }

  auto operator - ( const RealTime& a, const RealTime& b ) -> TimeDelta
  {
    const RealTime::NanosecondDuration ns{ a.TimeSinceEpoch() - b.TimeSinceEpoch() };
    const double seconds{ ns / 1e9 };
    return TimeDelta{ ( float )seconds };
  }

} // namespace Tac
