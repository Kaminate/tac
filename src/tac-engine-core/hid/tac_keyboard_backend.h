// TODO: The purpose of this file is...

#pragma once

#include "tac-engine-core/i18n/tac_localization.h" // Codepoint
#include "tac-engine-core/hid/tac_key.h" // Tac::Key
#include "tac-engine-core/shell/tac_shell_time.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/math/tac_vector2.h"

namespace Tac
{
  struct AppKeyboardApiBackend
  {
    enum class KeyState { Up = 0, Down = 1 };
    void SetKeyState( Key, KeyState );
    void SetCodepoint( Codepoint );
    void SetScreenspaceMousePos( v2 );
    void AddMouseWheelDelta( float );
    void Sync();

    bool GetIsPressed( Key );
    bool GetIsDepressed( Key );
    bool GetIsJustPressed( Key );
    bool GetIsJustDepressed( Key );
    auto GetHeldSeconds( Key ) -> RealTimeDelta;
    auto GetCodepoints() -> CodepointView;
    auto GetMouseWheelDelta() -> float; // units are magic
    auto GetMousePosScreenspace() -> v2i;
    auto GetMousePosDelta() -> v2i;

    struct KeyboardMouseState
    {
      using KeyStates = Array< AppKeyboardApiBackend::KeyState, ( int )Key::Count >;
      using KeyTimes = Array< RealTime, ( int )Key::Count >;
      using KeyToggles = Array< int, ( int )Key::Count >;
      float           mMouseWheel          {};
      v2i             mMousePosScreenspace {};
      KeyStates       mKeyStates           {};
      KeyTimes        mKeyTimes            {};
      RealTime        mTime                {};
      KeyToggles      mToggles             {};
      CodepointString mCodepointDelta      {};
    };

    KeyboardMouseState mCurr;
    KeyboardMouseState mPrev;
    static AppKeyboardApiBackend sGameKeyboardBackend;
    static AppKeyboardApiBackend sUIKeyboardBackend;
  };

} // namespace Tac
