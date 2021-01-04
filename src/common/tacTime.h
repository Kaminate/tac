#pragma once


#include "src/common/tacString.h"
#include <chrono>
namespace Tac
{
  const int TAC_FRAMES_PER_SECOND = 60;
  const float TAC_DELTA_FRAME_SECONDS = 1.0f / TAC_FRAMES_PER_SECOND;
  String FormatFrameTime( double seconds );

  using Clock = std::chrono::high_resolution_clock;
  using Nano = std::chrono::nanoseconds;
  using Timepoint = std::chrono::time_point< Clock, Nano >;

  Timepoint GetCurrentTime();

  // returns a - b
  float TimepointSubtractSeconds( Timepoint a, Timepoint b );
  float TimepointSubtractMiliseconds( Timepoint a, Timepoint b );

  float SecondsSince( Timepoint a );
}
