#include "tac-engine-core/shell/tac_shell_time.h"

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_util.h" // Join
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <chrono>
#endif

namespace Tac
{
  static auto FormatFrameTime( const double seconds ) -> String
  {
    struct TimeFormatter
    {
      void AddUnit( StringView name, double units, double unitsPerNext )
      {
        int unitDisplay { ( int )units };
        if( !unitDisplay )
          return;

        if( unitsPerNext )
          unitDisplay %= ( int )unitsPerNext;

        String text { ToString( unitDisplay ) + String( " " ) + String( name ) };
        if( unitDisplay > 1 )
          text += "s";

        lines.push_back( text );
      }
      Vector< String > lines;
    };
    const double miliseconds_per_second { 1000 };
    const double seconds_per_minute { 60 };
    const double minutes_per_hour { 60 };
    const double hours_per_day { 24 };
    const double days_per_week { 7 };
    const double weeks_per_year { 52 };

    const double miliseconds { seconds * miliseconds_per_second };
    const double minutes { seconds / seconds_per_minute };
    const double hours { minutes / minutes_per_hour };
    const double days { hours / hours_per_day };
    const double weeks { days / days_per_week };
    const double years { weeks / weeks_per_year };

    TimeFormatter timeFormatter;
    timeFormatter.AddUnit( "year", years, 0 );
    timeFormatter.AddUnit( "week", weeks, weeks_per_year );
    timeFormatter.AddUnit( "day", days, days_per_week );
    timeFormatter.AddUnit( "hour", hours, hours_per_day );
    timeFormatter.AddUnit( "minute", minutes, minutes_per_hour );
    timeFormatter.AddUnit( "second", seconds, seconds_per_minute );
    timeFormatter.AddUnit( "milisecond", miliseconds, miliseconds_per_second );

    String result { Join( timeFormatter.lines, " " ) };
    return result;
  }

  static_assert( is_same< std::chrono::nanoseconds::rep, RealTime::NanosecondDuration >::value );

  static RealTimer     sTimer         {};
  static GameTime      sElapsedTime   {};
  static RealTimeDelta sAccumulator   {};
  static GameFrame     sElapsedFrames {};

  // -----------------------------------------------------------------------------------------------

  RealTime::RealTime( NanosecondDuration ns )                 { mTimeSinceEpoch = ns; }

  auto RealTime::Now() -> RealTime
  {
    //       std::time_point                           std::duration      long long
    return { std::chrono::high_resolution_clock::now().time_since_epoch().count() };
  }

  auto RealTime::TimeSinceEpoch() const -> NanosecondDuration { return mTimeSinceEpoch; }

  // -----------------------------------------------------------------------------------------------

  auto RealTimeDelta::Format() const -> String                { return FormatFrameTime( mSeconds ); }
  RealTimeDelta::operator float() const                       { return mSeconds; }

  // -----------------------------------------------------------------------------------------------

  auto GameTime::Format() const -> String                     { return FormatFrameTime( mSeconds ); } 
  GameTime::operator double() const                           { return mSeconds; }

  // -----------------------------------------------------------------------------------------------

  auto GameTimeDelta::Format() const -> String                { return FormatFrameTime( mSeconds ); } 
  GameTimeDelta::operator float() const                       { return mSeconds; }

  // -----------------------------------------------------------------------------------------------

  void RealTimer::Start()
  {
    mStarted = true;
    mLastTick = RealTime::Now();
  }

  auto RealTimer::Tick() -> RealTimeDelta
  {
    TAC_ASSERT( mStarted );
    const RealTime now { RealTime::Now() };
    const RealTimeDelta seconds { now - mLastTick };
    mLastTick = now;
    return seconds;
  }

  bool RealTimer::IsRunning() const                           { return mStarted; }

  auto RealTimer::GetLastTick() const -> RealTime             { return mLastTick; }

  // -----------------------------------------------------------------------------------------------

  bool GameTimer::Update()
  {
    if( !sTimer.IsRunning() )
    {
      sTimer.Start();
      return true;
    }

    const RealTimeDelta dt{ sTimer.Tick() };
    sAccumulator.mSeconds = Fmod( sAccumulator.mSeconds + dt.mSeconds, TAC_DT.mSeconds * 2.0f );
    if( sAccumulator < TAC_DT )
      return false;

    sAccumulator -= RealTimeDelta{ .mSeconds { TAC_DT } };
    sElapsedTime += GameTimeDelta{ .mSeconds { TAC_DT } };
    sElapsedFrames++;
    return true;
  }
  auto GameTimer::GetElapsedTime() -> GameTime                { return sElapsedTime; }
  auto GameTimer::GetElapsedFrames() -> GameFrame             { return sElapsedFrames; }
  auto GameTimer::GetLastTick() -> RealTime                   { return sTimer.GetLastTick(); }

} // namespace Tac

auto Tac::operator += ( GameTime& stamp, const GameTimeDelta& diff ) -> GameTime&
{
  stamp.mSeconds += (double)diff.mSeconds;
  return stamp;
}

auto Tac::operator += ( GameTimeDelta& duration, const GameTimeDelta& other ) -> GameTimeDelta&
{
  duration.mSeconds += other.mSeconds;
  return duration;
}

auto Tac::operator -= ( GameTimeDelta& duration, const GameTimeDelta& other ) -> GameTimeDelta&
{
  duration.mSeconds -= other.mSeconds;
  return duration;
}

bool Tac::operator == ( const GameTime& a, const GameTime& b )
{
  return a.mSeconds == b.mSeconds;
}

bool Tac::operator < ( const GameTimeDelta& a, const GameTimeDelta& b)
{
  return a.mSeconds < b.mSeconds;
}

bool Tac::operator > ( const GameTimeDelta& a, const GameTimeDelta& b)
{
  return a.mSeconds > b.mSeconds;
}

auto Tac::operator - ( const GameTime& a, const GameTime& b ) -> GameTimeDelta
{
  return GameTimeDelta{ .mSeconds { float( a.mSeconds - b.mSeconds )} };
}

auto Tac::operator + ( const GameTimeDelta& a, const GameTime& b ) -> GameTime
{
  return GameTime{ .mSeconds { ( double )a.mSeconds + b.mSeconds } };
}

auto Tac::operator + ( const GameTime& a, const GameTimeDelta& b) -> GameTime
{
  return GameTime{ .mSeconds{ a.mSeconds + ( double )b.mSeconds } };
}

auto Tac::operator -= ( RealTime& a, const RealTimeDelta&b ) -> RealTime&
{
    a.mTimeSinceEpoch -= ( RealTime::NanosecondDuration )( b.mSeconds * 1e9 );
    return a;
}

auto Tac::operator - ( const RealTime& a, const RealTime& b ) -> RealTimeDelta
{
  const RealTime::NanosecondDuration ns{ a.TimeSinceEpoch() - b.TimeSinceEpoch() };
  const double seconds{ ns / 1e9 };
  return RealTimeDelta{ ( float )seconds };
}

auto Tac::operator -= ( RealTimeDelta& a, const RealTimeDelta& b) -> RealTimeDelta&
{
  a.mSeconds -= b.mSeconds;
  return a;
}

auto Tac::operator += ( RealTimeDelta& a, const RealTimeDelta& b) -> RealTimeDelta&
{
  a.mSeconds += b.mSeconds;
  return a;
}

