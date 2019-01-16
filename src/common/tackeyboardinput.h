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



struct TacKeyboardInput
{
  bool IsKeyDown( TacKey key );
  bool IsKeyUp( TacKey key );
  bool WasKeyDown( TacKey key );
  bool WasKeyUp( TacKey key );
  bool IsKeyJustDown( TacKey key );
  bool IsKeyJustUp( TacKey key );

  void DebugImgui();
  TacString GetKeysDownText();

  std::set< TacKey > mPrevDown;
  std::set< TacKey > mCurrDown;
};
