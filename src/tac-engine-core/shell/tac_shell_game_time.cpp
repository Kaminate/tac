#include "tac-engine-core/shell/tac_shell_game_time.h"

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

  //GameTime::GameTime( double s ) : mSeconds( s ) {}
  //GameTime::GameTime( int s ) : mSeconds( ( double )s ) {}

  GameTime::operator double() const { return mSeconds; }

  auto GameTime::Format() const -> String{ return FormatFrameTime( mSeconds ); } 

  // -----------------------------------------------------------------------------------------------

  //TimeDelta::TimeDelta( float s ) : mSeconds( s ) {}
  //TimeDelta::TimeDelta( int s ) : mSeconds( ( float )s ) {}

  auto TimeDelta::Format() const -> String{ return FormatFrameTime( mSeconds ); } 

  TimeDelta::operator float() const { return mSeconds; }

  // -----------------------------------------------------------------------------------------------

  auto operator += ( GameTime& stamp, const TimeDelta& diff ) -> GameTime&
  {
    stamp.mSeconds += (double)diff.mSeconds;
    return stamp;
  }

  auto operator += ( TimeDelta& duration, const TimeDelta& other ) -> TimeDelta&
  {
    duration.mSeconds += other.mSeconds;
    return duration;
  }

  auto operator -= ( TimeDelta& duration, const TimeDelta& other ) -> TimeDelta&
  {
    duration.mSeconds -= other.mSeconds;
    return duration;
  }



  bool operator == ( const GameTime& a, const GameTime& b ) { return a.mSeconds == b.mSeconds; }

  bool operator < ( const TimeDelta& a, const TimeDelta& b)
  {
    return a.mSeconds < b.mSeconds;
  }

  bool operator > ( const TimeDelta& a, const TimeDelta& b)
  {
    return a.mSeconds > b.mSeconds;
  }
  // -----------------------------------------------------------------------------------------------
  
  auto operator - ( const GameTime& a, const GameTime& b ) -> TimeDelta
  {
    return TimeDelta{ .mSeconds { float( a.mSeconds - b.mSeconds )} };
  }

  //TimeDelta operator * ( float a, const TimeDelta& b)
  //{
  //  return a * b.mSeconds;
  //}

  auto operator + ( const TimeDelta& a, const GameTime& b ) -> GameTime
  {
    return GameTime{ .mSeconds { ( double )a.mSeconds + b.mSeconds } };
  }

  auto operator + ( const GameTime& a, const TimeDelta& b) -> GameTime
  {
    return GameTime{ .mSeconds{ a.mSeconds + ( double )b.mSeconds } };
  }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac
