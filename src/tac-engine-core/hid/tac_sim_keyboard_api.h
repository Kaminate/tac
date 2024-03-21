#pragma once

#include "tac-engine-core/i18n/tac_localization.h" // Codepoint
#include "tac-engine-core/hid/tac_key.h" // Tac::Key
#include "tac-std-lib/containers/tac_span.h"
#include "tac-std-lib/math/tac_vector2.h" // v2

namespace Tac
{
  struct SimKeyboardApi
  {
    bool                IsPressed( Key );
    bool                IsDepressed( Key );
    bool                JustPressed( Key );
    bool                JustDepressed( Key );

    // This function returns a float instead of TimestampDifference
    // because it is a difference of Tac.Timepoint and not Tac.Timestamp.
    //
    // Returns 0 if the key is up
    float               HeldSeconds( Key );
    Span< Codepoint >   GetCodepoints();
    float               GetMouseWheelDelta(); // units are magic
    v2                  GetMousePosScreenspace();
    v2                  GetMousePosDelta();
  };
}

