
#pragma once

#include "src/common/math/tacVector2.h"
#include "src/common/tacString.h"

#include <cstdint>
namespace Tac
{

  struct ControllerInput;
  struct ControllerState;
  struct Controller;
  struct Errors;


  enum class ControllerButton
  {
    DPadUp,
    DPadLeft,
    DPadDown,
    DPadRight,
    Start,
    Back,
    LeftThumb,
    RightThumb,
    LeftShoulder,
    RightShoulder,
    A,
    B,
    X,
    Y,
    Count
  };
  const char* ToString( ControllerButton controllerButton );

  typedef uint16_t ControllerBitfield;

  ControllerBitfield ToBitfield( ControllerButton controllerButton );

  struct ControllerState
  {
    bool IsButtonDown( ControllerButton controllerButton );
    void DebugImgui();

    // x = [ 0, 1 ], y = [ 0, 1 ]
    v2 mLeftStick = {};
    v2 mRightStick = {};

    // [ 0, 1 ]
    float mLeftTrigger = {};
    float mRightTrigger = {};

    ControllerBitfield mButtons = {};
  };

  typedef int ControllerIndex;
  const ControllerIndex TAC_CONTROLLER_COUNT_MAX = 4;
  struct Controller
  {
    Controller();
    virtual ~Controller();
    bool IsButtonDown( ControllerButton controllerButton );
    bool IsButtonChanged( ControllerButton controllerButton );
    bool IsButtonJustPressed( ControllerButton controllerButton );
    bool IsButtonJustReleased( ControllerButton controllerButton );
    void DebugImgui();
    virtual void DebugImguiInner();
    ControllerIndex FindControllerIndex();

    ControllerState mControllerStatePrev = {};
    ControllerState mControllerStateCurr = {};
    bool mDebugging;
    String mName;
    ControllerInput* mInput = nullptr;
  };


  struct ControllerInput
  {
    static ControllerInput* Instance;
    ControllerInput();
    virtual ~ControllerInput();
    virtual void DebugImguiInner();
    void DebugImgui();
    void Update();
    virtual void UpdateInner();
    bool CanAddController();
    ControllerIndex GetConnectedControllerCount();
    ControllerIndex AddController( Controller* controller );

    Controller* mControllers[ TAC_CONTROLLER_COUNT_MAX ] = {};
    bool mDebugging;
    bool mForceIndexOverride = false;
    int mIndexOverride = 0;
  };

}

