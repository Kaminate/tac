// This file implements the input backend using the direct input library

#pragma once

// include windows.h before xinput.h or else you get 'no target architecture' errors
#include "src/shell/windows/tacWin32.h"
//#include "src/common/string/tacString.h"
#include "src/common/tacControllerInput.h"
//#include "src/common/tacErrorHandling.h"
//#include "src/common/containers/tacVector.h"

#define DIRECTINPUT_VERSION 0x0800 // must be before dinput.h
#include <dinput.h> // for other controller types

namespace Tac
{



  struct DirectInputPerController : public Controller
  {
    ~DirectInputPerController();
    void DebugImguiInner() override;

    DIDEVICEINSTANCE     mInstance = {};
    IDirectInputDevice8* mDevice = nullptr;
    DIJOYSTATE2          mJoystate = {};
  };

  struct XInput : public ControllerInput
  {
    void                      Init( Errors& );
    void                      UpdateInner() override;
    void                      DebugImguiInner() override;
    void                      EnumerateController( const DIDEVICEINSTANCE* );
    DirectInputPerController* FindDInputController( const DIDEVICEINSTANCE* );

    IDirectInput8*            directInput = nullptr;
    float                     mSecondsTillDisconver = 0;
    float                     mSecondsTillDiscoverMax = 1;
  };
}
