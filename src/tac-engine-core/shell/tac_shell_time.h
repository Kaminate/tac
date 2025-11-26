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

  struct GameTimeDelta
  {
    operator float() const;
    auto Format() const -> String;
    float mSeconds {};
  };

  struct RealTime
  {
    using NanosecondDuration = i64;

    RealTime() = default;
    RealTime( NanosecondDuration );
    auto TimeSinceEpoch() const -> NanosecondDuration;
    static auto Now() -> RealTime;
    NanosecondDuration mTimeSinceEpoch {};
  };

  struct RealTimeDelta
  {
    operator float() const;
    auto Format() const -> String;
    float mSeconds{};
  };

  struct GameTimer
  {
    static bool Update();
    static auto GetElapsedTime() -> GameTime;
    static auto GetElapsedFrames() -> GameFrame;
    static auto GetLastTick() -> RealTime;
  };

  struct RealTimer
  {
    void Start();
    auto Tick() -> RealTimeDelta;
    bool IsRunning() const;
    auto GetLastTick() const -> RealTime;

  private:
    RealTime mLastTick {};
    bool     mStarted  {};
  };

  inline const int           TAC_FPS { 60 };
  inline const GameTimeDelta TAC_DT  { 1.0f / TAC_FPS };

  auto operator += ( GameTime&, const GameTimeDelta& ) -> GameTime&;
  auto operator += ( GameTimeDelta&, const GameTimeDelta& ) -> GameTimeDelta&;
  auto operator -= ( GameTimeDelta&, const GameTimeDelta& ) -> GameTimeDelta&;
  bool operator == ( const GameTime&, const GameTime& );
  bool operator < ( const GameTimeDelta&, const GameTimeDelta& );
  bool operator > ( const GameTimeDelta&, const GameTimeDelta& );
  auto operator - ( const GameTime&, const GameTime& ) -> GameTimeDelta;
  auto operator + ( const GameTimeDelta&, const GameTime& ) -> GameTime;
  auto operator + ( const GameTime&, const GameTimeDelta& ) -> GameTime;

  auto operator - ( const RealTime&, const RealTime& ) -> RealTimeDelta;
  auto operator -= ( RealTime&, const RealTimeDelta& ) -> RealTime&;
  auto operator -= ( RealTimeDelta&, const RealTimeDelta& ) -> RealTimeDelta&;
  auto operator += ( RealTimeDelta&, const RealTimeDelta& ) -> RealTimeDelta&;


} // namespace Tac

