#pragma once

#include "src/common/string/tac_string.h"

namespace Tac
{
  struct Creation;
  struct Entity;
  struct Errors;
  struct Shell;
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

