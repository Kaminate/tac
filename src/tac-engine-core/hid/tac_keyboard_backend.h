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
  struct AppKeyboardApiBackend
  {
    enum class KeyState { Up = 0, Down = 1 };

    static void SetKeyState( Key, KeyState );
    static void SetCodepoint( Codepoint );
    static void SetMousePos( v2 screenspace );
    static void AddMouseWheelDelta( float );
    static void Sync();
  };
} // namespace Tac
