#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-engine-core/settings/tac_settings_node.h"

namespace Tac
{
  // you should be able to open this window from the main window menu bar
  // and then u can see each system ( graphics, physics, etc ) of level_editor->world->systems
  struct CreationSystemWindow
  {
    static void Update( SettingsNode );
    static bool sShowWindow;
  };
}

