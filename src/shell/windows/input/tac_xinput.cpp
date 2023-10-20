#include "src/shell/windows/input/tac_xinput.h"
#include "src/shell/windows/tac_win32.h"
#include "src/common/core/tac_preprocessor.h"
#include "src/common/math/tac_math.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/core/tac_error_handling.h"
#include "src/common/tac_common.h"
#include "src/common/input/tac_controller_input.h"

#include <libloaderapi.h>

#define DIRECTINPUT_VERSION 0x0800 // must be before dinput.h
#include <dinput.h> // for other controller types

#pragma comment( lib, "dxguid.lib" ) // IID_IDirectInput8A
#pragma comment( lib, "Dinput8.lib" )


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
    float                     mSecondsTillDiscover = 0;
    float                     mSecondsTillDiscoverMax = 1;
  };

  static BOOL CALLBACK enumDirectInputDevices( const DIDEVICEINSTANCE* pdidInstance,
                                               LPVOID pvRef )
  {
    XInput* xInput = ( XInput* )pvRef;
    xInput->EnumerateController( pdidInstance );
    return DIENUM_CONTINUE;
  }

  //static float ConvertDirectInputUnsigned( LONG inputVal )
  //{
  //  float result = ( float )inputVal;
  //  result /= 65535.0f;
  //  result = Saturate( result );
  //  return result;
  //}

  static float ConvertDirectInputSigned( LONG inputVal, float deadzonePercent )
  {
    float result = ( float )inputVal;
    result /= 65535.0f;
    result *= 2.0f;
    result -= 1.0f;
    float sign = result < 0 ? -1.0f : 1.0f;
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
    mDevice->Release();
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

  const char* GetDirectInput8CreateErr( HRESULT hr )
  {
    switch( hr )
    {
      case DIERR_BETADIRECTINPUTVERSION: return "beta ver"; 
      case DIERR_INVALIDPARAM: return "invalid param"; 
      case DIERR_OLDDIRECTINPUTVERSION: return "old ver"; 
      case DIERR_OUTOFMEMORY: return "oom"; 
    }

    return "???";
  }

  void XInput::Init( Errors& errors )
  {
    const HRESULT hr = DirectInput8Create( GetModuleHandleA(nullptr),
                                           DIRECTINPUT_VERSION,
                                           IID_IDirectInput8,
                                           ( LPVOID* )&mDirectInput,
                                           NULL );
    if( hr != DI_OK )
    {
      const char* errmsg = GetDirectInput8CreateErr( hr );
      TAC_RAISE_ERROR( errmsg, errors );
    }

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

  void XInput::UpdateInner()
  {

    mSecondsTillDiscover -= TAC_DELTA_FRAME_SECONDS;
    if( mSecondsTillDiscover < 0 )
    {
      mSecondsTillDiscover = mSecondsTillDiscoverMax;
      mDirectInput->EnumDevices( DI8DEVCLASS_GAMECTRL,
                                enumDirectInputDevices,
                                this,
                                DIEDFL_ATTACHEDONLY );
    }

    HRESULT hr = S_OK;
    for( int iController = 0; iController < TAC_CONTROLLER_COUNT_MAX; ++iController)
    {
      Controller* controller = mControllers[ iController ];
      if( !controller )
        continue;
      DirectInputPerController* directInputPerController = ( DirectInputPerController* )controller;
      if( mForceIndexOverride && ( iController != mIndexOverride ) )
        continue;

      IDirectInputDevice8* joystick = directInputPerController->mDevice;
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
        TAC_CRITICAL_ERROR_UNIMPLEMENTED;
      }
      else
      {
        TAC_ASSERT( hr != DIERR_INVALIDPARAM );
        TAC_ASSERT( hr != DIERR_NOTACQUIRED );
        TAC_ASSERT( hr != DIERR_NOTINITIALIZED );
        TAC_CRITICAL_ERROR_INVALID_CODE_PATH;
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
    HRESULT hr = mDirectInput->CreateDevice( pdidInstance->guidInstance,
                                            &joystick,
                                            NULL );
    TAC_ASSERT( SUCCEEDED( hr ) ); // ???
    controller = TAC_NEW DirectInputPerController;
    controller->mDevice = joystick;
    controller->mInstance = *pdidInstance;
    controller->mName = pdidInstance->tszInstanceName;

    hr = joystick->SetDataFormat( &c_dfDIJoystick2 );
    TAC_ASSERT( SUCCEEDED( hr ) );// ???

    hr = joystick->Acquire();
    TAC_ASSERT( SUCCEEDED( hr ) );// ???

    AddController( controller );
  }

  // API Functions

  void XInputInit( Errors& errors )
  {
    auto xInput = TAC_NEW XInput();
    xInput->Init( errors );
  }

} // namespace Tac
