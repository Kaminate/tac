// This file is the main API for joysticks.
// Controller* is hidden away

#pragma once

namespace Tac
{
  struct Controller;
  using ControllerIndex = int;
  const ControllerIndex TAC_CONTROLLER_COUNT_MAX{ 4 };

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

  struct ControllerApi
  {
    static const char* ButtonToString( ControllerButton );
    static void        UpdateJoysticks();
    static Controller* GetController( ControllerIndex );
    static bool        IsButtonJustPressed( ControllerButton, Controller* );
  };
} // namespace Tac

