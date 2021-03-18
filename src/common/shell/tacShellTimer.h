#pragma once

namespace Tac
{
  struct String;
  const int   TAC_FRAMES_PER_SECOND = 60;
  const float TAC_DELTA_FRAME_SECONDS = 1.0f / TAC_FRAMES_PER_SECOND;
  String      FormatFrameTime( double seconds );
  double      ShellGetElapsedSeconds();
  void        ShellTimerUpdate();
  bool        ShellTimerFrame();
}

