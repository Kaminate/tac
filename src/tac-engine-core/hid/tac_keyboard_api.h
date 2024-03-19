#pragma once

#include "tac-engine-core/i18n/tac_localization.h" // Codepoint
#include "tac-engine-core/hid/tac_key.h" // Tac::Key
#include "tac-std-lib/containers/tac_span.h"
#include "tac-std-lib/math/tac_vector2.h" // v2

namespace Tac
{
  struct KeyboardApi
  {
    static bool                IsPressed( Key );
    static bool                IsDepressed( Key );
    static bool                JustPressed( Key );
    static bool                JustReleased( Key );

    // This function returns a float instead of TimestampDifference
    // because it is a difference of Tac.Timepoint and not Tac.Timestamp.
    //
    // Returns 0 if the key is up
    static float               HeldSeconds( Key );
    static Span< Codepoint >   GetCodepoints();
    static float               GetMouseWheelDelta(); // units are magic
    static v2                  GetMousePosScreenspace();
    static v2                  GetMousePosDelta();
  };
}

