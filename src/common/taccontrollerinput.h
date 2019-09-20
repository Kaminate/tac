#pragma once

#include "common/math/tacVector2.h"
#include "common/tacString.h"
//#include "common/tacErrorHandling.h"

#include <cstdint>

struct TacControllerInput;
struct TacControllerState;
struct TacController;


enum class TacControllerButton
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
const char* TacToString( TacControllerButton controllerButton );

typedef uint16_t TacControllerBitfield;

TacControllerBitfield ToBitfield( TacControllerButton controllerButton );

struct TacControllerState
{
  bool IsButtonDown( TacControllerButton controllerButton );
  void DebugImgui();

  // x = [ 0, 1 ], y = [ 0, 1 ]
  v2 mLeftStick = {};
  v2 mRightStick = {};

  // [ 0, 1 ]
  float mLeftTrigger = {};
  float mRightTrigger = {};

  TacControllerBitfield mButtons = {};
};

typedef int TacControllerIndex;
const TacControllerIndex TAC_CONTROLLER_COUNT_MAX = 4;
struct TacController
{
  TacController();
  virtual ~TacController();
  bool IsButtonDown( TacControllerButton controllerButton );
  bool IsButtonChanged( TacControllerButton controllerButton );
  bool IsButtonJustPressed( TacControllerButton controllerButton );
  bool IsButtonJustReleased( TacControllerButton controllerButton );
  void DebugImgui();
  virtual void DebugImguiInner();
  TacControllerIndex FindControllerIndex();

  TacControllerState mControllerStatePrev = {};
  TacControllerState mControllerStateCurr = {};
  bool mDebugging;
  TacString mName;
  TacControllerInput* mInput = nullptr;
};


struct TacControllerInput
{
  static TacControllerInput* Instance;
  TacControllerInput();
  virtual ~TacControllerInput();
  virtual void DebugImguiInner();
  void DebugImgui();
  void Update();
  virtual void UpdateInner();
  bool CanAddController();
  TacControllerIndex GetConnectedControllerCount();
  TacControllerIndex AddController( TacController* controller );

  TacController* mControllers[ TAC_CONTROLLER_COUNT_MAX ] = {};
  bool mDebugging;
  bool mForceIndexOverride = false;
  int mIndexOverride = 0;
};
