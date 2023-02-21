#pragma once

#include "src/common/tac_desktop_window.h"
#include "src/common/tac_common.h"

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
    double                               mCreationSeconds = 0;
  };
}

