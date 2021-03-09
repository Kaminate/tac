#pragma once

#include "src/common/tacDesktopWindow.h"

namespace Tac
{
  struct Errors;
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

