#include "tac_keyboard_backend.h" // self-inc
#include "tac-desktop-app/tac_desktop_event.h"

namespace Tac::KeyboardBackend
{
  static KeyStates           sKeyStates;
  static KeyTimes            sKeyTimes;
  static KeyToggles          sKeyToggleCount;
  static v2                  sMousePosScreenspace;
  static Vector< Codepoint > sCodepointDelta;
  static std::mutex          sMutex;
  static float               sMouseWheel;
}

namespace Tac
{
  KeyboardBackend::KeyboardMouseState KeyboardBackend::sGameLogicPrev;
  KeyboardBackend::KeyboardMouseState KeyboardBackend::sGameLogicCurr;
  KeyboardBackend::KeyboardDelta      KeyboardBackend::sGameLogicDelta;

  void KeyboardBackend::SetKeyState( Key key, KeyState state )
  {
    TAC_SCOPE_GUARD( std::lock_guard, sMutex );
    if( const int i = ( int )key; sKeyStates[ i ] != state )
    {
      sKeyStates[ i ] = state;
      sKeyToggleCount[ i ]++;
      sKeyTimes[ i ] = Timepoint::Now();
    }
  }

  void KeyboardBackend::SetCodepoint( Codepoint codepoint )
  {
    TAC_SCOPE_GUARD( std::lock_guard, sMutex );
    sCodepointDelta.push_back( codepoint );
  }

  void KeyboardBackend::SetMousePos( v2 screenspace )
  {
    TAC_SCOPE_GUARD( std::lock_guard, sMutex );
    sMousePosScreenspace = screenspace;
  }

  void KeyboardBackend::SetMouseWheel( float wheelPos )
  {
    TAC_SCOPE_GUARD( std::lock_guard, sMutex );
    sMouseWheel = wheelPos;
  }

  void KeyboardBackend::UpdateGameLogicKeyState()
  {
    TAC_SCOPE_GUARD( std::lock_guard, sMutex );

    sGameLogicPrev = sGameLogicCurr;
    sGameLogicCurr = KeyboardMouseState
    {
      .mMouseWheel = sMouseWheel,
      .mMousePosScreenspace = sMousePosScreenspace,
      .mKeyStates = sKeyStates,
      .mKeyTimes = sKeyTimes,
      .mTime = Timepoint::Now(),
    };

    sGameLogicDelta = KeyboardDelta
    {
      .mToggles = sKeyToggleCount,
      .mCodepointDelta = sCodepointDelta,
    };

    sKeyToggleCount = {};
    sCodepointDelta = {};
  }
}

