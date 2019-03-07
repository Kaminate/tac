#pragma once
#include "tacString.h"
#include "tacLocalization.h"
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
};

struct TacKeyboardInput
{
  bool HasKeyJustBeenReleased( TacKey key );
  bool IsKeyJustDown( TacKey key );
  bool IsKeyDown( TacKey key );
  void DebugImgui();
  void DebugPrintWhenKeysChange();
  void SetIsKeyDown( TacKey key, bool isDown );
  void Frame();

  TacKeyboardInputFrame mCurr;
  TacKeyboardInputFrame mPrev;

  TacCodepoint mWMCharPressedHax = 0;
};
