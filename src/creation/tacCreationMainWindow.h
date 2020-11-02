#pragma once

#include "src/common/tacDesktopWindow.h"
//#include "src/common/tacErrorHandling.h"

namespace Tac
{
  struct UILayout;
  struct Shell;
  struct UIText;
  struct DesktopApp;
  struct Creation;
  struct UIRoot;
  struct UI2DDrawData;
  struct UIHierarchyNode;
  struct Texture;
  struct Errors;

  struct CreationMainWindow
  {
    CreationMainWindow();
    ~CreationMainWindow();
    static CreationMainWindow* Instance;
    void                       Init( Errors& );
    void                       Update( Errors& );
    void                       LoadTextures( Errors& );
    void                       ImGui();
    void                       ImGuiWindows();
    DesktopWindowHandle        mDesktopWindowHandle;
    UIRoot*                    mUIRoot = nullptr;
    UI2DDrawData*              mUI2DDrawData = nullptr;
    Texture*                   mIconWindow = nullptr;
    Texture*                   mIconClose = nullptr;
    Texture*                   mIconMaximize = nullptr;
    Texture*                   mIconMinimize = nullptr;
    UIHierarchyNode*           mGameObjectButton = nullptr;
    bool                       mAreTexturesLoaded = false;
    bool                       mAreLayoutsCreated = false;
  };

  const char* const gMainWindowName = "MainWindow";

}

