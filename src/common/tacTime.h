#pragma once

#include <chrono>
#include "common/tacString.h"
//#include "common/tacErrorHandling.h"

// 60( frames / sec )
// * 60( sec / min )
// * 60( min / hr )
// * 24( hr / day )
// * 7( day / week )
// * 52( week / year )
// = 1886976000, which is well under ( 2 ^ 32 ) - 1
// = 4294967295
const int TAC_FRAMES_PER_SECOND = 60;
const float TAC_DELTA_FRAME_SECONDS = 1.0f / TAC_FRAMES_PER_SECOND;
TacString TacFormatFrameTime( double seconds );

using TacClock = std::chrono::high_resolution_clock;
using TacNano = std::chrono::nanoseconds;
using TacTimepoint = std::chrono::time_point< TacClock, TacNano >;

struct TacTimerAux;
struct TacTimer
{
  void Start();
  void Tick();

  TacTimepoint mTimePoint;
  float mAccumulatedSeconds = 0;
};
