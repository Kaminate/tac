#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/tac_ints.h"

namespace Tac
{
  using GameFrame = u64;

  struct GameTime
  {
    operator double() const;
    auto Format() const -> String;
    double mSeconds {};
  };

  struct TimeDelta
  {
    operator float() const;
    auto Format() const -> String;
    float mSeconds {};
  };

  auto operator += ( GameTime&, const TimeDelta& ) -> GameTime&;
  auto operator += ( TimeDelta&, const TimeDelta& ) -> TimeDelta&;
  auto operator -= ( TimeDelta&, const TimeDelta& ) -> TimeDelta&;
  bool operator == ( const GameTime&, const GameTime& );
  bool operator < ( const TimeDelta&, const TimeDelta& );
  bool operator > ( const TimeDelta&, const TimeDelta& );
  auto operator - ( const GameTime&, const GameTime& ) -> TimeDelta;
  auto operator + ( const TimeDelta&, const GameTime& ) -> GameTime;
  auto operator + ( const GameTime&, const TimeDelta& ) -> GameTime;

} // namespace Tac

