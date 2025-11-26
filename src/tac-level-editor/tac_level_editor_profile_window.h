#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-ecs/tac_space.h"
#include "tac-engine-core/window/tac_window_handle.h"


namespace Tac
{
  struct CreationProfileWindow
  {
    static void                   Update( Errors& );
    static bool                   sShowWindow;
    static const char*            gProfileWindowName;
  };
}

