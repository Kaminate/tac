#pragma once


#include "src/common/tacString.h"
#include <chrono>
namespace Tac
{


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
  String FormatFrameTime( double seconds );

  using Clock = std::chrono::high_resolution_clock;
  using Nano = std::chrono::nanoseconds;
  using Timepoint = std::chrono::time_point< Clock, Nano >;

  Timepoint GetCurrentTime();

  // returns a - b
  float TimepointSubtractSeconds( Timepoint a, Timepoint b );
  float TimepointSubtractMiliseconds( Timepoint a, Timepoint b );
}
