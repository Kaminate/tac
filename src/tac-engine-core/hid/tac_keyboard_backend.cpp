#include "tac_keyboard_backend.h" // self-inc
#include "tac-desktop-app/tac_desktop_event.h"

namespace Tac::KeyboardBackend
{
  using KeyStates = Array< KeyState, ( int )Key::Count >;
  using KeyTimes = Array< Timepoint, ( int )Key::Count >;
  using KeyToggles = Array< int, ( int )Key::Count >;

  struct KeyboardMouseState
  {
    float     mMouseWheel;
    v2        mMousePosScreenspace;
    KeyStates mKeyStates;
    KeyTimes  mKeyTimes;
    Timepoint mTime;
  };

  struct KeyboardDelta
  {
    KeyToggles          mToggles;
    Vector< Codepoint > mCodepointDelta;
  };

  static KeyStates           sKeyStates;
  static KeyTimes            sKeyTimes;
  static KeyToggles          sKeyToggleCount;
  static v2                  sMousePosScreenspace;
  static Vector< Codepoint > sCodepointDelta;
  static std::mutex          sMutex;
  static bool                sModificationAllowed;
  static float               sMouseWheel;

  static KeyboardMouseState  sGameLogicPrev;
  static KeyboardMouseState  sGameLogicCurr;
  static KeyboardDelta       sGameLogicDelta;

  // -----------------------------------------------------------------------------------------------

  void SysApi::ApplyBegin()
  {
    sMutex.lock();
    sModificationAllowed = true;
  }

  void SysApi::ApplyEnd()
  {
    sModificationAllowed = false;
    sMutex.unlock();
  }

  void SysApi::SetKeyState( Key key, KeyState state )
  {
    TAC_ASSERT( sModificationAllowed );
    if( const int i = ( int )key; sKeyStates[ i ] != state )
    {
      sKeyStates[ i ] = state;
      sKeyToggleCount[ i ]++;
      sKeyTimes[ i ] = Timepoint::Now();
    }
  }

  void SysApi::SetCodepoint( Codepoint codepoint )
  {
    TAC_ASSERT( sModificationAllowed );
    sCodepointDelta.push_back( codepoint );
  }

  void SysApi::SetMousePos( v2 screenspace )
  {
    TAC_ASSERT( sModificationAllowed );
    sMousePosScreenspace = screenspace;
  }

  void SysApi::SetMouseWheel( float wheelPos )
  {
    TAC_ASSERT( sModificationAllowed );
    sMouseWheel = wheelPos;
  }

  // -----------------------------------------------------------------------------------------------

  void SymApi::Sync()
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

