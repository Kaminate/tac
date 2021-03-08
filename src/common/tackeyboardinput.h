#pragma once

#include "src/common/tacString.h"
#include "src/common/tacLocalization.h"
#include "src/common/math/tacVector2.h"
//#include "src/common/tacTime.h"

#include <set>

namespace Tac
{


  enum class Key
  {
    UpArrow, DownArrow, LeftArrow, RightArrow,
    Spacebar,
    Debug,
    Backspace,
    Delete,
    MouseLeft, MouseRight, MouseMiddle,
    Modifier,
    Escape,

    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    Tab,
    Backtick,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Count,
  };


  String ToString( Key key );


  struct KeyboardInputFrame
  {
    bool    IsKeyDown( Key key );
    String  GetPressedKeyDescriptions();
    bool    mCurrDown[ ( int )Key::Count ] = {};
    v2      mScreenspaceCursorPos = {};
    Errors  mScreenspaceCursorPosErrors;
    int     mMouseScroll = 0;
  };


  struct KeyboardInput
  {
    KeyboardInput();
    bool                HasKeyJustBeenReleased( Key key );
    bool                IsKeyJustDown( Key key );
    bool                IsKeyDown( Key key );
    void                DebugImgui();
    void                DebugPrintWhenKeysChange();
    void                SetIsKeyDown( Key key, bool isDown );

    // Called after input has been gathered, but before the game updates
    void                BeginFrame();
    void                EndFrame();


    KeyboardInputFrame  mCurr;
    KeyboardInputFrame  mPrev;

    Codepoint           mWMCharPressedHax = 0;

    // valid only if curr & prev cursor pos errors are empty
    v2                  mMouseDeltaPosScreenspace = {};

    int                 mMouseDeltaScroll = 0;
  };

  void                TryConsumeMouseMovement( double* );

  extern KeyboardInput gKeyboardInput;

}
