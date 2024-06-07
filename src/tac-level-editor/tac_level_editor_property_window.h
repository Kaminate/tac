#pragma once

#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-ecs/tac_space.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{

  struct CreationPropertyWindow
  {
    static void                    Update( Errors& );
    static bool                    sShowWindow;
  };
}
