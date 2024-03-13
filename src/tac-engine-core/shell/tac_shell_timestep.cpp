#include "tac_shell_timestep.h" // self-inc

#include "tac-engine-core/shell/tac_shell_timer.h"
#include "tac-std-lib/math/tac_math.h" // Max


namespace Tac
{

  static Timer               sTimer;
  static Timestamp           sElapsedTime;
  static TimestampDifference sAccumulator;
  static FrameIndex          sElapsedFrames = 0;

  bool                Timestep::Update()
  {
    if( !sTimer.IsRunning() )
    {
      sTimer.Start();
      return true;
    }

    const TimestampDifference dt = sTimer.Tick();
    sAccumulator = Fmod( ( float )sAccumulator + ( float )dt,
                         ( float )TAC_DELTA_FRAME_SECONDS * 2.0f );

    if( sAccumulator < TAC_DELTA_FRAME_SECONDS )
      return false;


    sAccumulator -= TAC_DELTA_FRAME_SECONDS;
    sElapsedTime += TAC_DELTA_FRAME_SECONDS;
    sElapsedFrames++;
    return true;
  }

  Timestamp           Timestep::GetElapsedTime()     { return sElapsedTime; }
  FrameIndex          Timestep::GetElapsedFrames()   { return sElapsedFrames; }
  TimestampDifference Timestep::GetAccumulatedTime() { return sAccumulator; }
  Timepoint           Timestep::GetLastTick()        { return sTimer.GetLastTick(); }

}
