#include "tac_keyboard_backend.h" // self-inc
#include "tac-desktop-app/tac_desktop_event.h"

Tac::KeyboardMouseState Tac::sGameLogicPrev;
Tac::KeyboardMouseState Tac::sGameLogicCurr;
Tac::KeyboardDelta      Tac::sGameLogicDelta;

namespace Tac
{
  static std::mutex          sMutex;
  static float               sMouseWheel;
  static v2                  sMousePosScreenspace;
  static KeyStates           sKeyStates;
  static KeyTimes            sKeyTimes;
  static KeyToggles          sKeyToggleCount;
  static Vector< Codepoint > sCodepointDelta;
}

void Tac::SetKeyState( Key key, KeyState state )
{
  TAC_SCOPE_GUARD( std::lock_guard, sMutex );
  if( const int i = ( int )key; sKeyStates[ i ] != state )
  {
    sKeyStates[ i ] = state;
    sKeyToggleCount[ i ]++;
    sKeyTimes[ i ] = Timepoint::Now();
  }
}

void Tac::SetCodepoint( Codepoint codepoint )
{
  TAC_SCOPE_GUARD( std::lock_guard, sMutex );
  sCodepointDelta.push_back( codepoint );
}

void Tac::SetMousePos( v2 screenspace )
{
  TAC_SCOPE_GUARD( std::lock_guard, sMutex );
  sMousePosScreenspace = screenspace;
}

void Tac::SetMouseWheel( float wheelPos )
{
  TAC_SCOPE_GUARD( std::lock_guard, sMutex );
  sMouseWheel = wheelPos;
}

void Tac::UpdateGameLogicKeyState()
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

