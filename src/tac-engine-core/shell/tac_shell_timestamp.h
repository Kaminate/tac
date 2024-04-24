#pragma once

namespace Tac
{
  struct String;
  struct TimestampDifference;

  struct Timestamp
  {
    Timestamp() = default;
    Timestamp( double );
    Timestamp( float ) = delete;
    void operator += ( const TimestampDifference& );
    operator double() const;
    double mSeconds { 0 };
  };

  struct TimestampDifference
  {
    TimestampDifference() = default;
    TimestampDifference( float );
    TimestampDifference( double ) = delete;
    void operator += ( const TimestampDifference& );
    void operator -= ( const TimestampDifference& );
    operator float() const;
    float mSeconds { 0 };
  };

  // TODO: rename to FormatSeconds
  String FormatFrameTime( double seconds );

  bool                operator == ( const Timestamp&, const Timestamp& );
  bool                operator < ( const TimestampDifference&, const TimestampDifference& );
  bool                operator > ( const TimestampDifference&, const TimestampDifference& );
  TimestampDifference operator - ( const Timestamp&, const Timestamp& );
  //TimestampDifference operator * ( float, const TimestampDifference& );
  Timestamp           operator + ( const TimestampDifference&, const Timestamp& );
  Timestamp           operator + ( const Timestamp&, const TimestampDifference& );

} // namespace Tac

