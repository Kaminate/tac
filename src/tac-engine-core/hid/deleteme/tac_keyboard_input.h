#pragma once

//#include "tac-std-lib/tac_core.h"
#include "tac-engine-core/i18n/tac_localization.h"
#include "tac-engine-core/input/tac_keyboard_key.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/math/tac_vector2.h"

namespace Tac { struct StackFrame; struct Timestamp; }

namespace Tac::HID
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
