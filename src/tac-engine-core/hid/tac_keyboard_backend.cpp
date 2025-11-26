#include "tac_keyboard_backend.h" // self-inc

#include "tac-desktop-app/desktop_event/tac_desktop_event.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-engine-core/shell/tac_shell_real_timer.h"

#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <mutex>
#endif

namespace Tac
{
  using KeyStates = Array< AppKeyboardApiBackend::KeyState, ( int )Key::Count >;
  using KeyTimes = Array< RealTime, ( int )Key::Count >;
  using KeyToggles = Array< int, ( int )Key::Count >;

  struct KeyboardMouseState
  {
    float           mMouseWheel          {};
    v2i             mMousePosScreenspace {};
    KeyStates       mKeyStates           {};
    KeyTimes        mKeyTimes            {};
    RealTime       mTime                {};
    KeyToggles      mToggles             {};
    CodepointString mCodepointDelta      {};
  };

  static KeyboardMouseState  sAppCurr;
  static KeyboardMouseState  sAppPrev;

  // -----------------------------------------------------------------------------------------------

  void AppKeyboardApiBackend::SetKeyState( Key key, KeyState state )
  {
    KeyState& currKeySate{ sAppCurr.mKeyStates[ ( int )key ] };
    if( currKeySate == state )
      return;

    currKeySate = state;
    sAppCurr.mToggles[ ( int )key ]++;
    sAppCurr.mKeyTimes[ ( int )key ] = RealTime::Now();
  }

  void AppKeyboardApiBackend::SetCodepoint( Codepoint codepoint )
  {
    sAppCurr.mCodepointDelta.push_back( codepoint );
  }

  void AppKeyboardApiBackend::SetScreenspaceMousePos( v2 screenspace )
  {
    sAppCurr.mMousePosScreenspace = screenspace;

    // prevent frame 1 jump when calling getmouse when calling GetMousePosDelta()
    if( static bool sInitialized; !sInitialized )
    {
      sInitialized = true;
      sAppPrev.mMousePosScreenspace = screenspace;
    }
  }

  void AppKeyboardApiBackend::AddMouseWheelDelta( float wheelDelta )
  {
    sAppCurr.mMouseWheel += wheelDelta;
  }

  // -----------------------------------------------------------------------------------------------

  void AppKeyboardApiBackend::Sync()
  {
    sAppPrev = sAppCurr;
    sAppCurr.mTime = RealTime::Now();
    sAppCurr.mToggles = {};
    sAppCurr.mCodepointDelta = {};
  }

  // -----------------------------------------------------------------------------------------------
  
  bool AppKeyboardApi::IsPressed( Key key )
  {
    return sAppCurr.mKeyStates[ ( int )key ] == AppKeyboardApiBackend::KeyState::Down;
  }
  bool AppKeyboardApi::IsDepressed( Key key )
  {
    if( key == Key::Myself ) { return true; }
    return sAppCurr.mKeyStates[ ( int )key ] == AppKeyboardApiBackend::KeyState::Up;
  }
  bool AppKeyboardApi::JustPressed( Key key )
  {
    const int toggleCount { sAppCurr.mToggles[ ( int )key ] };
    return IsPressed( key ) && toggleCount >= 1;
  }
  bool AppKeyboardApi::JustDepressed( Key key )
  {
    const int toggleCount { sAppCurr.mToggles[ ( int )key ] };
    return IsDepressed( key ) && toggleCount >= 1;
  }
  auto AppKeyboardApi::HeldSeconds( Key key ) -> TimeDelta
  {
    if( !IsPressed( key ) ) { return {}; }
    return sAppCurr.mTime - sAppCurr.mKeyTimes[ ( int )key ];
  }
  auto AppKeyboardApi::GetCodepoints() -> CodepointView
  {
    return sAppCurr.mCodepointDelta;
  }
  auto AppKeyboardApi::GetMouseWheelDelta() -> float
  {
    return sAppCurr.mMouseWheel - sAppPrev.mMouseWheel;
  }
  auto AppKeyboardApi::GetMousePosScreenspace() -> v2i
  {
    return sAppCurr.mMousePosScreenspace;
  }
  auto AppKeyboardApi::GetMousePosDelta() -> v2i
  {
    return sAppCurr.mMousePosScreenspace - sAppPrev.mMousePosScreenspace;
  }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac
