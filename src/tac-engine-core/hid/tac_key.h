#pragma once

namespace Tac{ struct StringView; }
namespace Tac
{
  enum class Key
  {
    Spacebar,
    Backspace,
    Delete,
    Modifier,
    Escape,
    Tab,
    Backtick,
    Myself,

    Debug, // ???

    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    Digit0, Digit1, Digit2, Digit3, Digit4, Digit5, Digit6, Digit7, Digit8, Digit9,

    UpArrow, DownArrow, LeftArrow, RightArrow,

    MouseLeft, MouseMiddle, MouseRight,

    Count,
  };
  
  StringView KeyToString( Key );
}
