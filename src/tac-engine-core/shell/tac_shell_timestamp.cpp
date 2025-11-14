#include "tac-engine-core/shell/tac_shell_timestamp.h"

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_util.h" // Join
#include "tac-std-lib/containers/tac_vector.h"

namespace Tac
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


  static String FormatFrameTime( const double seconds )
  {
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



  // -----------------------------------------------------------------------------------------------

  //Timestamp::Timestamp( double s ) : mSeconds( s ) {}
  //Timestamp::Timestamp( int s ) : mSeconds( ( double )s ) {}

  Timestamp::operator double() const { return mSeconds; }

  auto Timestamp::Format() const -> String{ return FormatFrameTime( mSeconds ); } 

  // -----------------------------------------------------------------------------------------------

  //TimeDuration::TimeDuration( float s ) : mSeconds( s ) {}
  //TimeDuration::TimeDuration( int s ) : mSeconds( ( float )s ) {}

  auto TimeDuration::Format() const -> String{ return FormatFrameTime( mSeconds ); } 

  TimeDuration::operator float() const { return mSeconds; }

  // -----------------------------------------------------------------------------------------------

  auto operator += ( Timestamp& stamp, const TimeDuration& diff ) -> Timestamp&
  {
    stamp.mSeconds += (double)diff.mSeconds;
    return stamp;
  }

  auto operator += ( TimeDuration& duration, const TimeDuration& other ) -> TimeDuration&
  {
    duration.mSeconds += other.mSeconds;
    return duration;
  }

  auto operator -= ( TimeDuration& duration, const TimeDuration& other ) -> TimeDuration&
  {
    duration.mSeconds -= other.mSeconds;
    return duration;
  }



  bool operator == ( const Timestamp& a, const Timestamp& b ) { return a.mSeconds == b.mSeconds; }

  bool operator < ( const TimeDuration& a, const TimeDuration& b)
  {
    return a.mSeconds < b.mSeconds;
  }

  bool operator > ( const TimeDuration& a, const TimeDuration& b)
  {
    return a.mSeconds > b.mSeconds;
  }
  // -----------------------------------------------------------------------------------------------
  
  auto operator - ( const Timestamp& a, const Timestamp& b ) -> TimeDuration
  {
    return TimeDuration{ .mSeconds { float( a.mSeconds - b.mSeconds )} };
  }

  //TimeDuration operator * ( float a, const TimeDuration& b)
  //{
  //  return a * b.mSeconds;
  //}

  auto operator + ( const TimeDuration& a, const Timestamp& b ) -> Timestamp
  {
    return Timestamp{ .mSeconds { ( double )a.mSeconds + b.mSeconds } };
  }

  auto operator + ( const Timestamp& a, const TimeDuration& b) -> Timestamp
  {
    return Timestamp{ .mSeconds{ a.mSeconds + ( double )b.mSeconds } };
  }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac
