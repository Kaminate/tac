#pragma once

#include "src/common/tacString.h"

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
    void                          ImGuiProfile();
    DesktopWindowHandle           mDesktopWindowHandle;
    UI2DDrawData*                 mUI2DDrawData = nullptr;

  };

  const char* const gProfileWindowName = "ProfileWindow";

}

