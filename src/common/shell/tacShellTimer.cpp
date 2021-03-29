#include "src/common/shell/tacShellTimer.h"
#include "src/common/containers/tacVector.h"
#include "src/common/tacUtility.h"

#include <chrono>

namespace Tac
{
  using Clock = std::chrono::high_resolution_clock;
  using Nano = std::chrono::nanoseconds;
  using Timepoint = std::chrono::time_point< Clock, Nano >;

  static double          mElapsedSeconds = 0;
  static Timepoint       mLastTick;
  static float           mAccumulatorSeconds = 0;

  static Timepoint GetCurrentTime() { return Clock::now(); }

  float TimepointSubtractSeconds( const Timepoint a, const Timepoint b )
  {
    return ( float )( a - b ).count() / ( float )1e9;
  }
  float TimepointSubtractMiliseconds( const Timepoint a, const Timepoint b )
  {
    return ( a - b ).count() / 1000000.0f;
  }
  float SecondsSince( const Timepoint a )
  {
    return TimepointSubtractSeconds( GetCurrentTime(), a );
  }


  struct TimeFormatter
  {
    void AddUnit( StringView name, double units, double unitsPerNext )
    {
      auto unitDisplay = ( int )units;
      if( !unitDisplay )
        return;
      if( unitsPerNext )
        unitDisplay %= ( int )unitsPerNext;
      String text = ToString( unitDisplay ) + String( " " ) + String( name );
      if( unitDisplay > 1 )
        text += "s";
      lines.push_back( text );
    }
    Vector< String > lines;
  };

  String FormatFrameTime( double seconds )
  {
    double miliseconds_per_second = 1000;
    double seconds_per_minute = 60;
    double minutes_per_hour = 60;
    double hours_per_day = 24;
    double days_per_week = 7;
    double weeks_per_year = 52;

    double miliseconds = seconds * miliseconds_per_second;
    double minutes = seconds / seconds_per_minute;
    double hours = minutes / minutes_per_hour;
    double days = hours / hours_per_day;
    double weeks = days / days_per_week;
    double years = weeks / weeks_per_year;

    TimeFormatter timeFormatter;
    timeFormatter.AddUnit( "year", years, 0 );
    timeFormatter.AddUnit( "week", weeks, weeks_per_year );
    timeFormatter.AddUnit( "day", days, days_per_week );
    timeFormatter.AddUnit( "hour", hours, hours_per_day );
    timeFormatter.AddUnit( "minute", minutes, minutes_per_hour );
    timeFormatter.AddUnit( "second", seconds, seconds_per_minute );
    timeFormatter.AddUnit( "milisecond", miliseconds, miliseconds_per_second );
    String result = Join( timeFormatter.lines, " " );
    return result;
  }


  double    ShellGetElapsedSeconds()
  {
    return mElapsedSeconds;
  }
  void      ShellTimerUpdate()
  {
    const Timepoint curTime = GetCurrentTime();
    mLastTick = mLastTick == Timepoint() ? curTime : mLastTick;
    mAccumulatorSeconds += TimepointSubtractSeconds( curTime, mLastTick );
    mLastTick = curTime;
  }
  bool      ShellTimerFrame()
  {
    if( mAccumulatorSeconds < TAC_DELTA_FRAME_SECONDS )
      return false;
    mAccumulatorSeconds -= TAC_DELTA_FRAME_SECONDS;
    mElapsedSeconds += TAC_DELTA_FRAME_SECONDS;
    return true;

  }
}
