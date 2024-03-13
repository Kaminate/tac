#include "tac-std-lib/shell/tac_shell_timestamp.h"

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_util.h" // Join
#include "tac-std-lib/containers/tac_vector.h"

namespace Tac
{
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

  String FormatFrameTime( const double seconds )
  {
    const double miliseconds_per_second = 1000;
    const double seconds_per_minute = 60;
    const double minutes_per_hour = 60;
    const double hours_per_day = 24;
    const double days_per_week = 7;
    const double weeks_per_year = 52;

    const double miliseconds = seconds * miliseconds_per_second;
    const double minutes = seconds / seconds_per_minute;
    const double hours = minutes / minutes_per_hour;
    const double days = hours / hours_per_day;
    const double weeks = days / days_per_week;
    const double years = weeks / weeks_per_year;

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


  // -----------------------------------------------------------------------------------------------

  Timestamp::Timestamp( double s ) : mSeconds( s ) {}

  Timestamp::operator double() const { return mSeconds; }

  void Timestamp::operator += ( const TimestampDifference& diff )
  {
    mSeconds += (double)diff.mSeconds;
  }

  // -----------------------------------------------------------------------------------------------

  TimestampDifference::TimestampDifference( float s ) : mSeconds( s ) {}

  TimestampDifference::operator float() const { return mSeconds; }

  void TimestampDifference::operator += ( const TimestampDifference& other )
  {
    mSeconds += other.mSeconds;
  }
  void TimestampDifference::operator -= ( const TimestampDifference& other )
  {
    mSeconds -= other.mSeconds;
  }

  // -----------------------------------------------------------------------------------------------

  bool operator == ( const Timestamp& a, const Timestamp& b ) { return a.mSeconds == b.mSeconds; }

  bool operator < ( const TimestampDifference& a, const TimestampDifference& b)
  {
    return a.mSeconds < b.mSeconds;
  }

  bool operator > ( const TimestampDifference& a, const TimestampDifference& b)
  {
    return a.mSeconds > b.mSeconds;
  }
  // -----------------------------------------------------------------------------------------------
  
  TimestampDifference operator - ( const Timestamp& a, const Timestamp& b )
  {
    return float(a.mSeconds - b.mSeconds);
  }

  //TimestampDifference operator * ( float a, const TimestampDifference& b)
  //{
  //  return a * b.mSeconds;
  //}

  Timestamp operator + ( const TimestampDifference& a, const Timestamp& b )
  {
    return  (double)a.mSeconds + b.mSeconds ;
  }

  Timestamp operator + ( const Timestamp& a, const TimestampDifference& b)
  {
    return  a.mSeconds + (double)b.mSeconds ;
  }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac
