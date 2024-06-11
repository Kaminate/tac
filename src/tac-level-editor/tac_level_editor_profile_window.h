#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-ecs/tac_space.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"

namespace Tac { struct Errors; }
namespace Tac
{
  struct CreationProfileWindow
  {
    static void                   Update( SimKeyboardApi, Errors& );
    static bool                   sShowWindow;
    static const char*            gProfileWindowName;
  };
}

