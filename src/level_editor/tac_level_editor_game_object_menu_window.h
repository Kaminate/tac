#pragma once

#include "src/common/system/tac_desktop_window.h"
#include "src/common/tac_common.h"
#include "src/common/shell/tac_shell_timer.h"

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

