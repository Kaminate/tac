#pragma once

#include "src/common/string/tac_string.h"
#include "src/space/tac_space.h"
#include "src/common/tac_common.h"

namespace Tac
{
  struct Creation;
  struct UI2DDrawData;
  struct UIHierarchyNode;
  struct UIRoot;

  struct CreationProfileWindow
  {
    CreationProfileWindow();
    ~CreationProfileWindow();
    static CreationProfileWindow* Instance;
    void                          Init( Errors& );
    void                          Update( Errors& );
    void                          ImGui();
    DesktopWindowHandle           mDesktopWindowHandle;
    UI2DDrawData*                 mUI2DDrawData = nullptr;
    bool                          mCloseRequested = false;
  };

  const char* const gProfileWindowName = "ProfileWindow";

}

