#pragma once

#include "tac-engine-core/shell/tac_shell_game_time.h"
#include "tac-std-lib/tac_ints.h"

namespace Tac
{
  struct RealTime
  {
    using NanosecondDuration = i64;

    RealTime() = default;
    RealTime( NanosecondDuration );
    auto TimeSinceEpoch() const -> NanosecondDuration;
    void operator -= ( TimeDelta );

    static auto Now() -> RealTime;

  private:
    NanosecondDuration mTimeSinceEpoch {};
  };

  auto operator - ( const RealTime&, const RealTime& ) -> TimeDelta;
} // namespace Tac

