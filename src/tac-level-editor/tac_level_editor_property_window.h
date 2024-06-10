#pragma once

#include "tac-ecs/tac_space.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{

  struct CreationPropertyWindow
  {
    static void                    Update( World*, Camera*, SettingsNode, Errors& );
    static bool                    sShowWindow;
  };
}
