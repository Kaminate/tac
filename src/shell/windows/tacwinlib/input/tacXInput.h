// This file implements the input backend using the direct input library

#pragma once

#include "src/common/tacControllerInput.h"

#define DIRECTINPUT_VERSION 0x0800 // must be before dinput.h
#include <dinput.h> // for other controller types

namespace Tac
{

  struct DirectInputPerController : public Controller
  {
    ~DirectInputPerController();
    void DebugImguiInner() override;

    DIDEVICEINSTANCE          mInstance = {};
    IDirectInputDevice8*      mDevice = nullptr;
    DIJOYSTATE2               mJoystate = {};
  };

  struct XInput : public ControllerInput
  {
    void                      Init( Errors& );
    void                      UpdateInner() override;
    void                      DebugImguiInner() override;
    void                      EnumerateController( const DIDEVICEINSTANCE* );
    DirectInputPerController* FindDInputController( const DIDEVICEINSTANCE* );

    IDirectInput8*            mDirectInput = nullptr;
    float                     mSecondsTillDisconver = 0;
    float                     mSecondsTillDiscoverMax = 1;
  };
}
