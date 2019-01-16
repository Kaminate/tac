#include "common/tacTime.h"
#include "common/tacUtility.h"
#include "common/containers/tacVector.h"

//#include <cmath>
//#include <iostream>
//#include "common/containers/tacVector.h"


struct TacTimeFormatter
{
  void AddUnit( const TacString& name, double units, double unitsPerNext )
  {
    auto unitDisplay = ( int )units;
    if( !unitDisplay )
      return;
    if( unitsPerNext )
      unitDisplay %= ( int )unitsPerNext;
    TacString text = TacToString( unitDisplay ) + " " + name;
    if( unitDisplay > 1 )
      text += "s";
    lines.push_back( text );
  }
  TacVector< TacString > lines;
};

TacString TacFormatFrameTime( double seconds )
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

  TacTimeFormatter timeFormatter;
  timeFormatter.AddUnit( "year", years, 0 );
  timeFormatter.AddUnit( "week", weeks, weeks_per_year );
  timeFormatter.AddUnit( "day", days, days_per_week );
  timeFormatter.AddUnit( "hour", hours, hours_per_day );
  timeFormatter.AddUnit( "minute", minutes, minutes_per_hour );
  timeFormatter.AddUnit( "second", seconds, seconds_per_minute );
  timeFormatter.AddUnit( "milisecond", miliseconds, miliseconds_per_second );
  TacString result = TacSeparateSpace( timeFormatter.lines );
  return result;
}


void TacTimer::Start()
{
  mTimePoint = TacClock::now();
}
void TacTimer::Tick()
{
  TacTimepoint timepoint = TacClock::now();
  TacNano nano = timepoint - mTimePoint;
  float elapsedSeconds = nano.count() / 1000000000.0f;
  mAccumulatedSeconds += elapsedSeconds;
  mTimePoint = timepoint;
}

