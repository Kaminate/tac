#pragma once

#include "tac-ecs/tac_space.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/string/tac_string.h"

#include "tac-level-editor/tac_level_editor_mouse_picking.h"

namespace Tac
{
  struct CreationGameWindow
  {
    static void Init( SettingsNode, Errors& );
    static void Update( World*, Camera*, Errors& );
    static void Render( World*, Camera*, Errors& );
    static void SetStatusMessage( StringView, TimestampDifference );
    static bool sShowWindow;
  };
}
