#pragma once

#include "src/common/string/tacString.h"

namespace Tac
{
  struct Errors;

  // you should be able to open this window from the main window menu bar
  // and then u can see each system ( graphics, physics, etc ) of creation->world->systems
  struct CreationSystemWindow
  {
    CreationSystemWindow();
    ~CreationSystemWindow();
    static CreationSystemWindow* Instance;
    void                         Init( Errors& );
    void                         Update( Errors& );
    void                         ImGui();
    DesktopWindowHandle          mDesktopWindowHandle;
    bool                         mCloseRequested = false;
    //String                       mSystemName;
  };

  const char* const gSystemWindowName = "SystemWindow";

}

