// TODO: The purpose of this file is...

#pragma once

#include "tac-engine-core/i18n/tac_localization.h" // Codepoint
#include "tac-engine-core/hid/tac_key.h" // Tac::Key
#include "tac-engine-core/shell/tac_shell_timer.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/math/tac_vector2.h"

namespace Tac
{

#if 0
  using KeyStates = Array< KeyState, ( int )Key::Count >;
  using KeyTimes = Array< Timepoint, ( int )Key::Count >;
  using KeyToggles = Array< int, ( int )Key::Count >;

  struct KeyboardMouseState
  {
    float     mMouseWheel;
    v2        mMousePosScreenspace;
    KeyStates mKeyStates;
    KeyTimes  mKeyTimes;
    Timepoint mTime;
  };

  struct KeyboardDelta
  {
    KeyToggles          mToggles;
    Vector< Codepoint > mCodepointDelta;
  };

  extern KeyboardMouseState sGameLogicPrev;
  extern KeyboardMouseState sGameLogicCurr;
  extern KeyboardDelta      sGameLogicDelta;

  // ... api wise, this could return a structure that holds
  // the lock, and has member functions that update state..
#endif
  

  struct SysKeyboardApiBackend
  {
    enum class KeyState { Up = 0, Down = 1 };

    void ApplyBegin();
    void SetKeyState( Key, KeyState );
    void SetCodepoint( Codepoint );
    void SetMousePos( v2 screenspace );
    void SetMouseWheel( float );
    void ApplyEnd();
  };

  struct SimKeyboardApiBackend
  {
    void Sync();
  };
}
