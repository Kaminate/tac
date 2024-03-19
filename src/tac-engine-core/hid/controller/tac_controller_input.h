// This file is the main API for joysticks.
// Controller* is hidden away

#pragma once

namespace Tac::Controller
{
  struct Controller;
  typedef int ControllerIndex;
  const ControllerIndex TAC_CONTROLLER_COUNT_MAX = 4;

  enum class ControllerButton
  {
    DPadUp, DPadLeft, DPadDown, DPadRight,
    Start,
    Back,
    LeftThumb, RightThumb,
    LeftShoulder, RightShoulder,
    A, B, X, Y,
    Count
  };

  const char* ToString( ControllerButton );
  void        UpdateJoysticks();
  Controller* GetController( ControllerIndex );
  bool        IsButtonJustPressed( ControllerButton, Controller* );

} // namespace Tac::Controller

