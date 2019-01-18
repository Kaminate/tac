#pragma once
#include "tacString.h"
#include <set>

enum class TacKey
{
  // TODO: move this shit up one level of indirection
  Up, Down, Left, Right,
  Space,
  Debug,
  Back,
  MouseLeft, MouseRight, MouseMiddle,
  Modifier,

  // TODO: make keyboard mapping not suck?
  Q,
  E,
  F,
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
  void BeforePoll();

  TacKeyboardInputFrame mCurr;
  TacKeyboardInputFrame mPrev;
};
