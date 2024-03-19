#include "tac_keyboard_api.h" // self-inc
#include "tac_keyboard_backend.h"


namespace Tac
{
  static KeyboardBackend::KeyState  GetKeyState( Key key )
  {
    return sGameLogicCurr.mKeyStates[ ( int )key ];
  }

  static int       GetKeyToggleCount( Key key )
  {
    return sGameLogicDelta.mToggles[ ( int )key ];
  }

  static Timepoint GetKeyTime( Key key )      
  {
  }

  static bool IsKeyInState( Key key, KeyboardBackend::KeyState keyState )
  {
    return keyState == sGameLogicCurr.mKeyStates[ ( int )key ];
  }


  bool KeyboardApi::IsPressed( Key key ) 
  {
    return IsKeyInState( key, KeyboardBackend::KeyState::Down );
  }

  bool KeyboardApi::JustReleased( Key key ) 
  {
    const KeyState keyState = GetKeyState( key );
    const int keyToggleCount = GetKeyToggleCount( key );
    return IsKeyInState( key, KeyboardBackend::KeyState::Up ) && keyToggleCount >= 1;
  }

  bool KeyboardApi::JustPressed( Key key ) 
  {
    const KeyState keyState = GetKeyState( key );
    const int keyToggleCount = GetKeyToggleCount( key );
    return keyState == KeyState::Down && keyToggleCount >= 1;
  }

  float KeyboardApi::HeldSeconds( Key key )
  {
    const KeyState keyState = GetKeyState( key );
    if( keyState == KeyState::Up )
      return 0;

    const Timepoint keyTime = sGameLogicCurr.mKeyTimes[ ( int )key ];
    return sGameLogicCurr.mTime - keyTime;
  }

  Span< Codepoint >   KeyboardApi::GetCodepoints()
  {
    return
    {
      sGameLogicDelta.mCodepointDelta.data(),
      sGameLogicDelta.mCodepointDelta.size()
    };
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

