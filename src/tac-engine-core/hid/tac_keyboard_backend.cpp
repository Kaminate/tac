#include "tac_keyboard_backend.h" // self-inc

#include "tac-desktop-app/desktop_event/tac_desktop_event.h"
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-engine-core/hid/tac_sys_keyboard_api.h"

#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <mutex>
#endif

namespace Tac
{
  using KeyStates = Array< AppKeyboardApiBackend::KeyState, ( int )Key::Count >;
  using KeyTimes = Array< Timepoint, ( int )Key::Count >;
  using KeyToggles = Array< int, ( int )Key::Count >;

  struct KeyboardMouseState
  {
    float           mMouseWheel          {};
    v2i             mMousePosScreenspace {};
    KeyStates       mKeyStates           {};
    KeyTimes        mKeyTimes            {};
    Timepoint       mTime                {};
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
    sAppCurr.mKeyTimes[ ( int )key ] = Timepoint::Now();
  }

  void AppKeyboardApiBackend::SetCodepoint( Codepoint codepoint )
  {
    sAppCurr.mCodepointDelta.push_back( codepoint );
  }

  void AppKeyboardApiBackend::SetMousePos( v2 screenspace )
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
    sAppCurr.mTime = Timepoint::Now();
    sAppCurr.mToggles = {};
    sAppCurr.mCodepointDelta = {};
  }


  // -----------------------------------------------------------------------------------------------

  
  bool                AppKeyboardApi::IsPressed( Key key )
  {
    return sAppCurr.mKeyStates[ ( int )key ] == AppKeyboardApiBackend::KeyState::Down;
  }
  bool                AppKeyboardApi::IsDepressed( Key key )
  {
    if( key == Key::Myself ) { return true; }
    return sAppCurr.mKeyStates[ ( int )key ] == AppKeyboardApiBackend::KeyState::Up;
  }
  bool                AppKeyboardApi::JustPressed( Key key )
  {
    const int toggleCount { sAppCurr.mToggles[ ( int )key ] };
    return IsPressed( key ) && toggleCount >= 1;
  }
  bool                AppKeyboardApi::JustDepressed( Key key )
  {
    const int toggleCount { sAppCurr.mToggles[ ( int )key ] };
    return IsDepressed( key ) && toggleCount >= 1;
  }
  TimestampDifference AppKeyboardApi::HeldSeconds( Key key )
  {
    if( !IsPressed( key ) ) { return {}; }
    return sAppCurr.mTime - sAppCurr.mKeyTimes[ ( int )key ];
  }
  CodepointView       AppKeyboardApi::GetCodepoints()
  {
    return sAppCurr.mCodepointDelta;
  }
  float               AppKeyboardApi::GetMouseWheelDelta()
  {
    return sAppCurr.mMouseWheel - sAppPrev.mMouseWheel;
  }
  v2i                 AppKeyboardApi::GetMousePosScreenspace()
  {
    return sAppCurr.mMousePosScreenspace;
  }
  v2i                 AppKeyboardApi::GetMousePosDelta()
  {
    return sAppCurr.mMousePosScreenspace - sAppPrev.mMousePosScreenspace;
  }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac
