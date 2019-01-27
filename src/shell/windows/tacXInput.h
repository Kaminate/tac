// This file implements the input backend using the direct input library

#pragma once

#include "tacWindows.h" // include windows.h before xinput.h or else you get 'no target architecture' errors
#include "common/tacString.h"
#include "common/taccontrollerinput.h"
#include "common/tacErrorHandling.h"
#include "common/containers/tacVector.h"

#define DIRECTINPUT_VERSION 0x0800 // must be before dinput.h
#include <dinput.h> // for other controller types


struct TacDirectInputPerController : public TacController
{
  ~TacDirectInputPerController();
  void DebugImguiInner() override;

  DIDEVICEINSTANCE mInstance = {};
  IDirectInputDevice8* mDivice = nullptr;
  DIJOYSTATE2 mJoystate = {};
};

struct TacXInput : public TacControllerInput
{
  TacXInput( HINSTANCE hInstance, TacErrors& errors );
  ~TacXInput();
  void UpdateInner() override;
  void DebugImguiInner() override;
  void EnumerateController( const DIDEVICEINSTANCE* pdidInstance );
  TacDirectInputPerController* FindDInputController( const DIDEVICEINSTANCE* mDeviceInstance );

  IDirectInput8* directInput = nullptr;
  float mSecondsTillDisconver = 0;
  float mSecondsTillDiscoverMax = 1;
};
