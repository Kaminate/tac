#pragma once

#include "src/common/tacString.h"

namespace Tac
{
  struct Errors;

  // you should be able to open this window from the main window menu bar
  // and then u can see each system ( graphics, physics, etc ) of creation->world->systems
  struct CreationSystemWindow
  {
    static CreationSystemWindow* Instance;
    CreationSystemWindow();
    ~CreationSystemWindow();
    void                Init( Errors& errors );
    void                Update( Errors& errors );
    void                ImGui();
    DesktopWindowHandle mDesktopWindowHandle;
    String              mSystemName;
  };

  const String gSystemWindowName = "SystemWindow";

}

