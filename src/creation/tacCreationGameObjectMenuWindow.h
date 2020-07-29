
#pragma once

#include "src/common/tacDesktopWindow.h"
#include "src/common/tacErrorHandling.h"

namespace Tac
{
  struct UILayout;
  struct Shell;
  struct UIText;
  /*struct DesktopWindow*/;
  struct DesktopApp;
  struct Creation;
  struct UIRoot;
  struct UI2DDrawData;
  struct UIHierarchyNode;
  struct Texture;
  struct CreationMainWindow;

  struct CreationGameObjectMenuWindow
  {
    CreationGameObjectMenuWindow();
    ~CreationGameObjectMenuWindow();
    void Init( Errors& errors );
    void CreateLayouts();
    void Update( Errors& errors );

    static CreationGameObjectMenuWindow* Instance;

    DesktopWindowHandle mDesktopWindowHandle;
    //DesktopWindow* mDesktopWindow = nullptr;
    //UIRoot* mUIRoot = nullptr;
    UI2DDrawData* mUI2DDrawData = nullptr;
    double mCreationSeconds;
  };


}

