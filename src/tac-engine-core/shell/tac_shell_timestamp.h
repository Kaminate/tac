#pragma once

namespace Tac
{
  struct String;
  struct TimeDuration;

  struct Timestamp
  {
    //Timestamp() = default;
    //Timestamp( double );
    //Timestamp( int );
    //Timestamp( float ) = delete;
    operator double() const;
    auto Format() const -> String;
    double mSeconds {};
  };

  struct TimeDuration
  {
    //TimeDuration() = default;
    //TimeDuration( float );
    //TimeDuration( int );
    //void operator += ( const TimeDuration& );
    //void operator -= ( const TimeDuration& );
    operator float() const;
    auto Format() const -> String;
    float mSeconds {};
  };

  auto operator += ( Timestamp&, const TimeDuration& ) -> Timestamp&;
  auto operator += ( TimeDuration&, const TimeDuration& ) -> TimeDuration&;
  auto operator -= ( TimeDuration&, const TimeDuration& ) -> TimeDuration&;
  bool operator == ( const Timestamp&, const Timestamp& );
  bool operator < ( const TimeDuration&, const TimeDuration& );
  bool operator > ( const TimeDuration&, const TimeDuration& );
  auto operator - ( const Timestamp&, const Timestamp& ) -> TimeDuration;
  auto operator + ( const TimeDuration&, const Timestamp& ) -> Timestamp;
  auto operator + ( const Timestamp&, const TimeDuration& ) -> Timestamp;

} // namespace Tac

