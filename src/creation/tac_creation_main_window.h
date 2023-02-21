#pragma once

#include "src/common/tac_desktop_window.h"
#include "src/common/tac_common.h"
//#include "src/common/tac_error_handling.h"

namespace Tac
{

  struct Creation;
  struct DesktopApp;
  struct Texture;
  struct UI2DDrawData;
  struct UIHierarchyNode;
  struct UILayout;
  struct UIRoot;
  struct UIText;

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
    void                       ImGuiPrefabs();
    void                       ImGuiSaveAs();
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
    bool                       mCloseRequested = false;
  };

  const char* const gMainWindowName = "MainWindow";

}

