#pragma once

#include "src/common/string/tacString.h"
#include "src/common/tacLocalization.h"
#include "src/common/math/tacVector2.h"

//#include <set>

namespace Tac
{
  struct StackFrame;


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


  String ToString( Key );


  struct KeyboardInputFrame
  {
    bool    IsKeyDown( Key );
    String  GetPressedKeyDescriptions();
    bool    mCurrDown[ ( int )Key::Count ] = {};
    v2      mScreenspaceCursorPos = {};
    //Errors  mScreenspaceCursorPosErrors;
    int     mMouseScroll = 0;
  };


  struct KeyboardInput
  {
    KeyboardInput();
    bool               HasKeyJustBeenReleased( Key );
    bool               IsKeyJustDown( Key );
    bool               IsKeyDown( Key );
    void               DebugImgui();
    void               DebugPrintWhenKeysChange();
    void               SetIsKeyDown( Key, bool );

    // Called after input has been gathered, but before the game updates
    void               BeginFrame();
    void               EndFrame();


    KeyboardInputFrame mCurr;
    KeyboardInputFrame mPrev;

    Codepoint          mWMCharPressedHax = 0;

    // valid only if curr & prev cursor pos errors are empty
    v2                 mMouseDeltaPos = {};

    int                mMouseDeltaScroll = 0;
  };

  //                   sets to 0 if not consumed
  void                 TryConsumeMouseMovement( double*, StackFrame );
  extern KeyboardInput gKeyboardInput;

}
