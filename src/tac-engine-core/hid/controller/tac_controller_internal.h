// This file holds controller data shared by different platforms
// It is included by tac_xinput.h

#pragma once

//#include "tac-std-lib/tac_core.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/tac_ints.h"
#include "tac_controller_input.h"


namespace Tac::Controller
{

  struct ControllerInput;
  struct ControllerState;
  struct Controller;

  using ControllerBitfield = u16;

  ControllerBitfield ToBitfield( ControllerButton );

  struct ControllerState
  {
    bool                   IsButtonDown( ControllerButton );
    void                   DebugImgui();

    // x = [ 0, 1 ], y = [ 0, 1 ]
    v2                      mLeftStick    {};
    v2                      mRightStick   {};

    // [ 0, 1 ]
    float                   mLeftTrigger  {};
    float                   mRightTrigger {};
    ControllerBitfield      mButtons      {};
  };

  struct Controller
  {
    Controller();
    virtual                 ~Controller();
    bool                    IsButtonDown( ControllerButton );
    bool                    IsButtonChanged( ControllerButton );
    bool                    IsButtonJustPressed( ControllerButton );
    bool                    IsButtonJustReleased( ControllerButton );
    void                    DebugImgui();
    virtual void            DebugImguiInner();
    ControllerIndex         FindControllerIndex();

    ControllerState         mControllerStatePrev  {};
    ControllerState         mControllerStateCurr  {};
    bool                    mDebugging            {};
    String                  mName                 {};
    ControllerInput*        mInput                {};
  };


  struct ControllerInput
  {
    ControllerInput();
    static ControllerInput* Instance;
    virtual                 ~ControllerInput();
    virtual void            DebugImguiInner();
    void                    DebugImgui();
    void                    Update();
    virtual void            UpdateInner();
    bool                    CanAddController();
    ControllerIndex         GetConnectedControllerCount();
    ControllerIndex         AddController( Controller* );
    Controller*             GetController( ControllerIndex );

    Controller*             mControllers[ TAC_CONTROLLER_COUNT_MAX ]  {};
    bool                    mDebugging                                {};
    bool                    mForceIndexOverride                       {};
    int                     mIndexOverride                            {};
  };

} // namespace Tac::Controller
