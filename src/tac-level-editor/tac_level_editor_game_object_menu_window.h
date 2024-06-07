// wtf does this file do?

#pragma once

#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
  struct CreationGameObjectMenuWindow
  {
    CreationGameObjectMenuWindow();
    ~CreationGameObjectMenuWindow();
    void                                 Init( Errors& );
    void                                 Update( Errors& );
    static CreationGameObjectMenuWindow* Instance;
    WindowHandle                         mWindowHandle;
    Timestamp                            mCreationSeconds;
  };
}

