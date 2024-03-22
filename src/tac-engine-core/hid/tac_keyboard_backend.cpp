#include "tac_keyboard_backend.h" // self-inc

#include "tac-desktop-app/tac_desktop_event.h"
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-engine-core/hid/tac_sys_keyboard_api.h"

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

  void SimApi::Sync()
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

namespace Tac
{
  using namespace KeyboardBackend;

  bool                SimKeyboardApi::IsPressed( Key key )
  {
    return sGameLogicCurr.mKeyStates[ ( int )key ] == KeyState::Down;
  }

  bool                SimKeyboardApi::IsDepressed( Key key )
  {
    if( key == Key::Myself )
      return true; // :(

    return sGameLogicCurr.mKeyStates[ ( int )key ] == KeyState::Up;
  }

  bool                SimKeyboardApi::JustPressed( Key key )
  {
    const int toggleCount = sGameLogicDelta.mToggles[ ( int )key ] >= 1;
    return IsPressed( key ) && toggleCount;
  }

  bool                SimKeyboardApi::JustDepressed( Key key )
  {
    if( key == Key::Myself )
      return true; // :(

    const int toggleCount = sGameLogicDelta.mToggles[ ( int )key ] >= 1;
    return IsDepressed( key ) && toggleCount;
  }


  // This function returns a float instead of TimestampDifference
  // because it is a difference of Tac.Timepoint and not Tac.Timestamp.
  //
  // Returns 0 if the key is up
  float               SimKeyboardApi::HeldSeconds( Key key )
  {
    if( !IsPressed( key ) )
      return 0;

    return sGameLogicCurr.mTime - sGameLogicCurr.mKeyTimes[ ( int )key ];
  }

  Span< Codepoint >   SimKeyboardApi::GetCodepoints()
  {
    return
    {
      sGameLogicDelta.mCodepointDelta.data(),
      sGameLogicDelta.mCodepointDelta.size()
    };
  }

  float               SimKeyboardApi::GetMouseWheelDelta() // units are magic
  {
    return sGameLogicCurr.mMouseWheel - sGameLogicPrev.mMouseWheel;
  }

  v2                  SimKeyboardApi::GetMousePosScreenspace()
  {
    return sGameLogicCurr.mMousePosScreenspace;
  }

  v2                  SimKeyboardApi::GetMousePosDelta()
  {
    return sGameLogicCurr.mMousePosScreenspace - sGameLogicPrev.mMousePosScreenspace;
  }
  // -----------------------------------------------------------------------------------------------

  void SysKeyboardApi::TestTest() {} // no-op

}
