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
  static bool                sModificationAllowed;
  static float               sMouseWheel;
}

namespace Tac
{
  KeyboardBackend::KeyboardMouseState KeyboardBackend::sGameLogicPrev;
  KeyboardBackend::KeyboardMouseState KeyboardBackend::sGameLogicCurr;
  KeyboardBackend::KeyboardDelta      KeyboardBackend::sGameLogicDelta;

  void KeyboardBackend::ApplyBegin()
  {
    sMutex.lock();
    sModificationAllowed = true;
  }

  void KeyboardBackend::ApplyEnd()
  {
    sModificationAllowed = false;
    sMutex.unlock();
  }

  void KeyboardBackend::SetKeyState( Key key, KeyState state )
  {
    TAC_ASSERT( sModificationAllowed );
    if( const int i = ( int )key; sKeyStates[ i ] != state )
    {
      sKeyStates[ i ] = state;
      sKeyToggleCount[ i ]++;
      sKeyTimes[ i ] = Timepoint::Now();
    }
  }

  void KeyboardBackend::SetCodepoint( Codepoint codepoint )
  {
    TAC_ASSERT( sModificationAllowed );
    sCodepointDelta.push_back( codepoint );
  }

  void KeyboardBackend::SetMousePos( v2 screenspace )
  {
    TAC_ASSERT( sModificationAllowed );
    sMousePosScreenspace = screenspace;
  }

  void KeyboardBackend::SetMouseWheel( float wheelPos )
  {
    TAC_ASSERT( sModificationAllowed );
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

