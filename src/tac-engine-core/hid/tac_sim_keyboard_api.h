#pragma once

#include "tac-engine-core/i18n/tac_localization.h" // Codepoint
#include "tac-engine-core/hid/tac_key.h" // Tac::Key
#include "tac-std-lib/containers/tac_span.h"
#include "tac-std-lib/math/tac_vector2i.h" // v2i

namespace Tac
{
  struct SimKeyboardApi
  {
    bool                IsPressed( Key ) const;
    bool                IsDepressed( Key ) const;
    bool                JustPressed( Key ) const;
    bool                JustDepressed( Key ) const;

    // This function returns a float instead of TimestampDifference
    // because it is a difference of Tac.Timepoint and not Tac.Timestamp.
    //
    // Returns 0 if the key is up
    float               HeldSeconds( Key ) const;
    Span< Codepoint >   GetCodepoints() const;
    float               GetMouseWheelDelta() const; // units are magic
    v2i                 GetMousePosScreenspace() const;
    v2i                 GetMousePosDelta() const;
  };
}

