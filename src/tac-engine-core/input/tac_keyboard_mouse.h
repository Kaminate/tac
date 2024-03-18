#pragma once

#include "tac-engine-core/i18n/tac_localization.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/math/tac_vector2.h"

namespace Tac { struct StackFrame; struct Timestamp; }

namespace Tac::Keyboard
{
  enum class Key
  {
    UpArrow, DownArrow, LeftArrow, RightArrow,
    Spacebar,
    Debug,
    Backspace,
    Delete,
    Modifier,
    Escape,

    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    Tab,
    Backtick,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Count,
  };

  struct GameLogicState
  {
    // return number of times released;
    int Released( Key );
    int Pressed( Key );
  };

  struct KeyState
  {
    bool mIsDown = false;
    int mToggleCount = 0;
  };

  struct GameLogicState
  {
    KeyState mKeyStates[ ( int )Key::Count ]{};
  };

  struct PlatformState
  {
    void KeyDownEvent( Key );
    void KeyUpEvent( Key );
    void UpdateGameLogicState( GameLogicState* );

    KeyState mKeyStates[ ( int )Key::Count ]{};
  };
}

namespace Tac
{
};

  String    ToString( Key );

  bool      KeyboardHasKeyJustBeenReleased( Key );
  bool      KeyboardIsKeyJustDown( Key );
  bool      KeyboardIsKeyDown( Key );
  bool      KeyboardWasKeyDown( Key );
  void      KeyboardSetIsKeyDown( Key, bool );

  void      KeyboardDebugImgui();
  void      KeyboardDebugPrintWhenKeysChange();
  Codepoint KeyboardGetWMCharPressedHax();
  void      KeyboardSetWMCharPressedHax( Codepoint );


  //        Called after input has been gathered, but before the game updates
  void      KeyboardBeginFrame();
  void      KeyboardEndFrame();


} // namespace Tac::Keyboard

namespace Tac::Mouse
{

  enum class Button
  {
    MouseLeft,
    MouseRight,
    MouseMiddle,
    Count
  };

  String    ToString( Button );

  v2        GetMouseDeltaPos();
  void      MouseWheelEvent( int delta );

  //        sets to 0 if not consumed
  void      TryConsumeMouseMovement( Timestamp*, const StackFrame& );

  int       GetMouseDeltaScroll();
  v2        GetScreenspaceCursorPos();
  void      SetScreenspaceCursorPos( v2 );

  void      MouseBeginFrame();
  void      MouseEndFrame();

  bool      ButtonJustBeenReleased( Button );
  bool      ButtonJustDown( Button );
  bool      ButtonIsDown( Button );
  bool      ButtonWasDown( Button );
  void      ButtonSetIsDown( Button, bool );

} // namespace Tac::Mouse
