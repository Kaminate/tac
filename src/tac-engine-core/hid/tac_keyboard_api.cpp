#include "tac_keyboard_api.h" // self-inc
#include "tac_keyboard_backend.h"


namespace Tac
{
  using KeyboardBackend::KeyState;
  using KeyboardBackend::sGameLogicCurr;
  using KeyboardBackend::sGameLogicPrev;
  using KeyboardBackend::sGameLogicDelta;

  static KeyState  GetKeyState( Key key )
  {
    return sGameLogicCurr.mKeyStates[ ( int )key ];
  }

  static int       GetKeyToggleCount( Key key )
  {
    return sGameLogicDelta.mToggles[ ( int )key ];
  }

  static Timepoint GetKeyTime( Key key )      
  {
    return sGameLogicCurr.mKeyTimes[ ( int )key ];
  }

  static bool IsKeyInState( Key key, KeyState keyState )
  {
    return keyState == GetKeyState( key );
  }

  // -----------------------------------------------------------------------------------------------

  bool KeyboardApi::IsPressed( Key key ) 
  {
    return IsKeyInState( key, KeyState::Down );
  }

  bool KeyboardApi::IsDepressed( Key key )
  {
    return IsKeyInState( key, KeyState::Up );
  }

  bool KeyboardApi::JustReleased( Key key ) 
  {
    const int keyToggleCount = GetKeyToggleCount( key );
    return IsDepressed( key ) && keyToggleCount >= 1;
  }

  bool KeyboardApi::JustPressed( Key key ) 
  {
    const int keyToggleCount = GetKeyToggleCount( key );
    return IsPressed( key ) && keyToggleCount >= 1;
  }

  float KeyboardApi::HeldSeconds( Key key )
  {
    if( IsDepressed( key ) )
      return 0;

    const Timepoint keyTime = GetKeyTime( key );
    const Timepoint curTime = sGameLogicCurr.mTime;
    const Timepoint::NanosecondDuration ns
      = curTime.TimeSinceEpoch()
      - keyTime.TimeSinceEpoch();
    return ( float )( ns / 1e9 );
  }

  Span< Codepoint >   KeyboardApi::GetCodepoints()
  {
    Vector< Codepoint >& vec = sGameLogicDelta.mCodepointDelta;
    return { vec.data(), vec.size() };
  }

  float               KeyboardApi::GetMouseWheelDelta()
  {
    return
      sGameLogicCurr.mMouseWheel -
      sGameLogicPrev.mMouseWheel;
  }

  v2                  KeyboardApi::GetMousePosScreenspace()
  {
    return sGameLogicCurr.mMousePosScreenspace;
  }

  v2                  KeyboardApi::GetMousePosDelta()
  {
    return
      sGameLogicCurr.mMousePosScreenspace -
      sGameLogicPrev.mMousePosScreenspace;
  }

} // namespace Tac::HID

