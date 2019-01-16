#include "tacXInput.h"
#include "tacPreprocessor.h"
#include "tacMath.h"
#include "tacShell.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#pragma comment( lib, "Dinput8.lib" )


float ConvertDirectInputUnsigned( LONG inputVal )
{
  float result = ( float )inputVal;
  result /= 65535.0f;
  result = TacSaturate( result );
  return result;
}
float ConvertDirectInputSigned( LONG inputVal, float deadzonePercent )
{
  float result = ( float )inputVal;
  result /= 65535.0f;
  result *= 2.0f;
  result -= 1.0f;
  float sign = std::signbit( result )?-1.0f:1.0f;
  result *= sign;
  result -= deadzonePercent;
  result /= 1.0f - deadzonePercent;
  result = TacSaturate( result );
  result *= sign;
  return result;
}

TacControllerState TacToControllerState( const DIJOYSTATE2& js )
{
  float deadzone = 0.1f;
  TacControllerState controllerState = {};
  controllerState.mLeftStick.x = ConvertDirectInputSigned( js.lX, deadzone );
  //controllerState.mLeftStick.y = ConvertDirectInputSigned( js.lY, deadzone );


  return controllerState;
}

TacDirectInputPerController::~TacDirectInputPerController()
{
  mDivice->Release();
}
void TacDirectInputPerController::DebugImguiInner()
{
  if( !ImGui::CollapsingHeader( "Direct Input" ) )
    return;
  DIJOYSTATE2 js = mJoystate;
  TacString s;
  for( auto b : js.rgbButtons )
    s += b?'1':'0';
  ImGui::Text( "lX: %i", js.lX );
  ImGui::Text( "lY: %i", js.lY );
  ImGui::Text( "lZ: %i", js.lZ );
  ImGui::Text( "lRx: %i", js.lRx );
  ImGui::Text( "lRy: %i", js.lRy );
  ImGui::Text( "lRz: %i", js.lRz );
  ImGui::Text( "rglSlider: %i %i", js.rglSlider[ 0 ], js.rglSlider[ 1 ] );
  ImGui::Text( "rgdwPOV: %i %i %i %i", js.rgdwPOV[ 0 ], js.rgdwPOV[ 1 ], js.rgdwPOV[ 2 ], js.rgdwPOV[ 3 ] );
  ImGui::Text( s );
  ImGui::Text( "lVX: %i", js.lVX );
  ImGui::Text( "lVY: %i", js.lVY );
  ImGui::Text( "lVZ: %i", js.lVZ );
  ImGui::Text( "lVRx: %i", js.lVRx );
  ImGui::Text( "lVRy: %i", js.lVRy );
  ImGui::Text( "lVRz: %i", js.lVRz );
  ImGui::Text( "rglVSlider: %i %i", js.rglVSlider[ 0 ], js.rglVSlider[ 1 ] );
  ImGui::Text( "lAX: %i", js.lAX );
  ImGui::Text( "lAY: %i", js.lAY );
  ImGui::Text( "lAZ: %i", js.lAZ );
  ImGui::Text( "lARx: %i", js.lARx );
  ImGui::Text( "lARy: %i", js.lARy );
  ImGui::Text( "lARz: %i", js.lARz );
  ImGui::Text( "rglASlider: %i %i", js.rglASlider[ 0 ], js.rglASlider[ 1 ] );
  ImGui::Text( "lFX: %i", js.lFX );
  ImGui::Text( "lFY: %i", js.lFY );
  ImGui::Text( "lFZ: %i", js.lFZ );
  ImGui::Text( "lFRx: %i", js.lFRx );
  ImGui::Text( "lFRy: %i", js.lFRy );
  ImGui::Text( "lFRz: %i", js.lFRz );
  ImGui::Text( "rglFSlider: %i %i", js.rglFSlider[ 0 ], js.rglFSlider[ 1 ] );
}

TacXInput::TacXInput( HINSTANCE hInstance, TacErrors& errors )
{
  REFIID riidltf = IID_IDirectInput8;
  HRESULT hr = DirectInput8Create( hInstance, DIRECTINPUT_VERSION, riidltf, ( LPVOID* )&directInput, NULL );
  switch( hr )
  {
  case DI_OK: break;
  case DIERR_BETADIRECTINPUTVERSION: errors = "DIERR_BETADIRECTINPUTVERSION"; return;
  case DIERR_INVALIDPARAM:errors = "DIERR_INVALIDPARAM"; return;
  case DIERR_OLDDIRECTINPUTVERSION:errors = "DIERR_OLDDIRECTINPUTVERSION"; return;
  case DIERR_OUTOFMEMORY:errors = "DIERR_OUTOFMEMORY"; return;
    TacInvalidDefaultCase( hr );
  }
}
TacXInput::~TacXInput()
{
}
TacDirectInputPerController* TacXInput::FindDInputController( const DIDEVICEINSTANCE* mDeviceInstance )
{
  for( TacController* controller : mControllers )
  {
    if( !controller )
      continue;
    TacDirectInputPerController* directInputPerController = ( TacDirectInputPerController * )controller;
    if( mDeviceInstance->guidInstance == directInputPerController->mInstance.guidInstance )
      return directInputPerController;
  }
  return nullptr;
}
void TacXInput::DebugImguiInner()
{
  ImGui::DragFloat( "Seconds till discover cur", &mSecondsTillDisconver );
  ImGui::DragFloat( "Seconds till discover max", &mSecondsTillDiscoverMax );
}
void TacXInput::UpdateInner()
{
  mSecondsTillDisconver -= TAC_DELTA_FRAME_SECONDS;
  if( mSecondsTillDisconver < 0 )
  {
    auto enumDirectInputDevices = [](
      const DIDEVICEINSTANCE* pdidInstance,
      LPVOID pvRef )->BOOL
    {
      TacXInput* xInput = ( TacXInput* )pvRef;
      xInput->EnumerateController( pdidInstance );
      return DIENUM_CONTINUE;
    };
    mSecondsTillDisconver = mSecondsTillDiscoverMax;
    directInput->EnumDevices(
      DI8DEVCLASS_GAMECTRL,
      enumDirectInputDevices,
      this,
      DIEDFL_ATTACHEDONLY
    );
  }

  HRESULT hr;

  int iController = 0;
  for( TacController* controller : mControllers )
  {
    if( !controller )
      continue;
    TacDirectInputPerController* directInputPerController = ( TacDirectInputPerController* )controller;
    if( mForceIndexOverride && ( iController != mIndexOverride ) )
      continue;


    OnDestruct( iController++ );
    TacController* controller = mControllers[ iController ];

    IDirectInputDevice8* joystick = directInputPerController->mDivice;
    hr = joystick->Poll();
    if( FAILED( hr ) )
    {
      // ???
    }


    DIJOYSTATE2 js = {};
    hr = joystick->GetDeviceState( sizeof( DIJOYSTATE2 ), &js );
    if( hr == S_OK )
    {
      directInputPerController->mJoystate = js;
    }
    else if( hr == E_PENDING )
    {
      // dont assign js while pending
    }
    else if( hr == DIERR_INPUTLOST )
    {
      TacAssertMessage( "todo" );
    }
    else
    {
      TacAssert( hr != DIERR_INVALIDPARAM );
      TacAssert( hr != DIERR_NOTACQUIRED );
      TacAssert( hr != DIERR_NOTINITIALIZED );
      TacInvalidCodePath;
    }

    TacControllerState controllerState = TacToControllerState( js );
    controller->mControllerStateCurr = controllerState;





  }
}
void TacXInput::EnumerateController( const DIDEVICEINSTANCE* pdidInstance )
{
  TacDirectInputPerController* controller = FindDInputController( pdidInstance );
  if( controller )
    return;
  IDirectInputDevice8* joystick = nullptr;
  HRESULT hr = directInput->CreateDevice( pdidInstance->guidInstance, &joystick, NULL );
  TacAssert( SUCCEEDED( hr ) ); // ???
  controller = new TacDirectInputPerController();
  controller->mDivice = joystick;
  controller->mInstance = *pdidInstance;
  controller->mName = pdidInstance->tszInstanceName;

  hr = joystick->SetDataFormat( &c_dfDIJoystick2 );
  TacAssert( SUCCEEDED( hr ) );// ???

  hr = joystick->Acquire();
  TacAssert( SUCCEEDED( hr ) );// ???

  AddController( controller );
}
