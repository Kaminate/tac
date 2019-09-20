#pragma once
#include "common/tacString.h"
#include "common/tacLocalization.h"
#include "common/math/tacVector2.h"
#include <set>

enum class TacKey
{
  UpArrow, DownArrow, LeftArrow, RightArrow,
  Spacebar,
  Debug,
  Backspace,
  Delete,
  MouseLeft, MouseRight, MouseMiddle,
  Modifier,

  A, B, C, D, E, F, G, H, I, J, K, L, M,
  N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

  Tab,
  Backtick,
  F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
  Count,
};


TacString ToString( TacKey key );


struct TacKeyboardInputFrame
{
  bool IsKeyDown( TacKey key );
  TacString GetPressedKeyDescriptions();

  std::set< TacKey > mCurrDown;
  v2 mScreenspaceCursorPos = {};
  int mMouseScroll = 0;
};

struct TacKeyboardInput
{
  static TacKeyboardInput* Instance;
  TacKeyboardInput();
  bool HasKeyJustBeenReleased( TacKey key );
  bool IsKeyJustDown( TacKey key );
  bool IsKeyDown( TacKey key );
  void DebugImgui();
  void DebugPrintWhenKeysChange();
  void SetIsKeyDown( TacKey key, bool isDown );

  // Called after input has been gathered, but before the game updates
  void BeginFrame();
  void EndFrame();

  TacKeyboardInputFrame mCurr;
  TacKeyboardInputFrame mPrev;

  TacCodepoint mWMCharPressedHax = 0;
  v2 mMouseDeltaPosScreenspace = {};
  int mMouseDeltaScroll = 0;
};
