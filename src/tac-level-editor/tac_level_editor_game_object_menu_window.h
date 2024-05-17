#pragma once

#include "tac-engine-core/window/tac_window_handle.h"
#include "src/common/tac_core.h"
#include "src/common/shell/tac_shell_timestep.h"

namespace Tac
{
  struct CreationGameObjectMenuWindow
  {
    CreationGameObjectMenuWindow();
    ~CreationGameObjectMenuWindow();
    void                                 Init( Errors& );
    void                                 Update( Errors& );
    static CreationGameObjectMenuWindow* Instance;
    DesktopWindowHandle                  mDesktopWindowHandle;
    Timestamp                            mCreationSeconds;
  };
}

