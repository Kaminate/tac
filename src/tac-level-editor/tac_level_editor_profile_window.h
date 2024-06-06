#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-ecs/tac_space.h"
#include "tac-engine-core/window/tac_window_handle.h"

namespace Tac { struct Errors; }
namespace Tac
{
  struct CreationProfileWindow
  {
    static void                   Update( const SimKeyboardApi , Errors& );
    static bool                   mCloseRequested;
    static const char*            gProfileWindowName;
  };
}

