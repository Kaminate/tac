#include "src/shell/windows/tacXInput.h"
#include "src/shell/windows/tacWindowsApp2.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/math/tacMath.h"
#include "src/common/tacShell.h"
#include "src/common/tacTime.h"


#include <algorithm>
#include <cmath>
#include <iostream>

#pragma comment( lib, "Dinput8.lib" )

namespace Tac
{



  float ConvertDirectInputUnsigned( LONG inputVal )
  {
    float result = ( float )inputVal;
    result /= 65535.0f;
    result = Saturate( result );
    return result;
  }
  float ConvertDirectInputSigned( LONG inputVal, float deadzonePercent )
  {
    float result = ( float )inputVal;
    result /= 65535.0f;
    result *= 2.0f;
    result -= 1.0f;
    float sign = std::signbit( result ) ? -1.0f : 1.0f;
    result *= sign;
    result -= deadzonePercent;
    result /= 1.0f - deadzonePercent;
    result = Saturate( result );
    result *= sign;
    return result;
  }

  ControllerState ToControllerState( const DIJOYSTATE2& js )
  {
    float deadzone = 0.1f;
    ControllerState controllerState = {};
    controllerState.mLeftStick.x = ConvertDirectInputSigned( js.lX, deadzone );
    //controllerState.mLeftStick.y = ConvertDirectInputSigned( js.lY, deadzone );


    return controllerState;
  }

  DirectInputPerController::~DirectInputPerController()
  {
    mDivice->Release();
  }
  void DirectInputPerController::DebugImguiInner()
  {
    //if( !ImGui::CollapsingHeader( "Direct Input" ) )
    //  return;
    //DIJOYSTATE2 js = mJoystate;
    //String s;
    //for( auto b : js.rgbButtons )
    //  s += b?'1':'0';
    //ImGui::Text( "lX: %i", js.lX );
    //ImGui::Text( "lY: %i", js.lY );
    //ImGui::Text( "lZ: %i", js.lZ );
    //ImGui::Text( "lRx: %i", js.lRx );
    //ImGui::Text( "lRy: %i", js.lRy );
    //ImGui::Text( "lRz: %i", js.lRz );
    //ImGui::Text( "rglSlider: %i %i", js.rglSlider[ 0 ], js.rglSlider[ 1 ] );
    //ImGui::Text( "rgdwPOV: %i %i %i %i", js.rgdwPOV[ 0 ], js.rgdwPOV[ 1 ], js.rgdwPOV[ 2 ], js.rgdwPOV[ 3 ] );
    //ImGui::Text( s );
    //ImGui::Text( "lVX: %i", js.lVX );
    //ImGui::Text( "lVY: %i", js.lVY );
    //ImGui::Text( "lVZ: %i", js.lVZ );
    //ImGui::Text( "lVRx: %i", js.lVRx );
    //ImGui::Text( "lVRy: %i", js.lVRy );
    //ImGui::Text( "lVRz: %i", js.lVRz );
    //ImGui::Text( "rglVSlider: %i %i", js.rglVSlider[ 0 ], js.rglVSlider[ 1 ] );
    //ImGui::Text( "lAX: %i", js.lAX );
    //ImGui::Text( "lAY: %i", js.lAY );
    //ImGui::Text( "lAZ: %i", js.lAZ );
    //ImGui::Text( "lARx: %i", js.lARx );
    //ImGui::Text( "lARy: %i", js.lARy );
    //ImGui::Text( "lARz: %i", js.lARz );
    //ImGui::Text( "rglASlider: %i %i", js.rglASlider[ 0 ], js.rglASlider[ 1 ] );
    //ImGui::Text( "lFX: %i", js.lFX );
    //ImGui::Text( "lFY: %i", js.lFY );
    //ImGui::Text( "lFZ: %i", js.lFZ );
    //ImGui::Text( "lFRx: %i", js.lFRx );
    //ImGui::Text( "lFRy: %i", js.lFRy );
    //ImGui::Text( "lFRz: %i", js.lFRz );
    //ImGui::Text( "rglFSlider: %i %i", js.rglFSlider[ 0 ], js.rglFSlider[ 1 ] );
  }

  XInput::XInput( HINSTANCE hInstance, Errors& errors )
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
        TAC_INVALID_DEFAULT_CASE( hr );
    }
  }
  XInput::~XInput()
  {
  }
  DirectInputPerController* XInput::FindDInputController( const DIDEVICEINSTANCE* mDeviceInstance )
  {
    for( Controller* controller : mControllers )
    {
      if( !controller )
        continue;
      DirectInputPerController* directInputPerController = ( DirectInputPerController * )controller;
      if( mDeviceInstance->guidInstance == directInputPerController->mInstance.guidInstance )
        return directInputPerController;
    }
    return nullptr;
  }
  void XInput::DebugImguiInner()
  {
    //ImGui::DragFloat( "Seconds till discover cur", &mSecondsTillDisconver );
    //ImGui::DragFloat( "Seconds till discover max", &mSecondsTillDiscoverMax );
  }

  static BOOL CALLBACK enumDirectInputDevices( const DIDEVICEINSTANCE* pdidInstance,
                                               LPVOID pvRef )
  {
    XInput* xInput = ( XInput* )pvRef;
    xInput->EnumerateController( pdidInstance );
    return DIENUM_CONTINUE;
  }
  void XInput::UpdateInner()
  {
    return;

    mSecondsTillDisconver -= TAC_DELTA_FRAME_SECONDS;
    if( mSecondsTillDisconver < 0 )
    {
      mSecondsTillDisconver = mSecondsTillDiscoverMax;
      directInput->EnumDevices( DI8DEVCLASS_GAMECTRL,
                                enumDirectInputDevices,
                                this,
                                DIEDFL_ATTACHEDONLY );
    }
    HRESULT hr = S_OK;
    int iController = 0;
    for( Controller* controller : mControllers )
    {
      if( !controller )
        continue;
      DirectInputPerController* directInputPerController = ( DirectInputPerController* )controller;
      if( mForceIndexOverride && ( iController != mIndexOverride ) )
        continue;

      TAC_ON_DESTRUCT( iController++ );
      Controller* controller = mControllers[ iController ];

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
        TAC_ASSERT_MESSAGE( "todo" );
      }
      else
      {
        TAC_ASSERT( hr != DIERR_INVALIDPARAM );
        TAC_ASSERT( hr != DIERR_NOTACQUIRED );
        TAC_ASSERT( hr != DIERR_NOTINITIALIZED );
        TAC_INVALID_CODE_PATH;
      }

      ControllerState controllerState = ToControllerState( js );
      controller->mControllerStateCurr = controllerState;





    }
  }
  void XInput::EnumerateController( const DIDEVICEINSTANCE* pdidInstance )
  {
    DirectInputPerController* controller = FindDInputController( pdidInstance );
    if( controller )
      return;
    IDirectInputDevice8* joystick = nullptr;
    HRESULT hr = directInput->CreateDevice( pdidInstance->guidInstance, &joystick, NULL );
    TAC_ASSERT( SUCCEEDED( hr ) ); // ???
    controller = new DirectInputPerController();
    controller->mDivice = joystick;
    controller->mInstance = *pdidInstance;
    controller->mName = pdidInstance->tszInstanceName;

    hr = joystick->SetDataFormat( &c_dfDIJoystick2 );
    TAC_ASSERT( SUCCEEDED( hr ) );// ???

    hr = joystick->Acquire();
    TAC_ASSERT( SUCCEEDED( hr ) );// ???

    AddController( controller );
  }
}
