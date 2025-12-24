#pragma once

#include "tac-engine-core/shell/tac_shell_time.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-ecs/tac_space.h"

namespace Tac
{
  struct CreationGameWindow
  {
    static void Init( Errors& );
    static void Update( Errors& );
    static void Render( World*, const Camera*, Errors& );
    static void SetStatusMessage( StringView, GameTimeDelta );
    static void DebugImGui( Errors& );
    static auto GetWindowHandle() -> WindowHandle;
    static bool sShowWindow;
  };
}
