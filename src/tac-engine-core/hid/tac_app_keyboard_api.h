#pragma once

#include "tac-engine-core/i18n/tac_localization.h" // Codepoint
#include "tac-engine-core/hid/tac_key.h" // Tac::Key
#include "tac-engine-core/shell/tac_shell_timestamp.h" // TimeDuration
#include "tac-std-lib/containers/tac_span.h"
#include "tac-std-lib/math/tac_vector2i.h" // v2i

namespace Tac
{
  struct AppKeyboardApi
  {
    static bool                IsPressed( Key );
    static bool                IsDepressed( Key );
    static bool                JustPressed( Key );
    static bool                JustDepressed( Key );

    // This function returns a float instead of TimeDuration
    // because it is a difference of Tac.Timepoint and not Tac.Timestamp.
    //
    // Returns 0 if the key is up
    static TimeDuration HeldSeconds( Key );
    static CodepointView       GetCodepoints();
    static float               GetMouseWheelDelta(); // units are magic
    static v2i                 GetMousePosScreenspace();
    static v2i                 GetMousePosDelta();
  };
}

