#pragma once

#include "tac-engine-core/window/tac_window_handle.h"

#include "tac-ecs/tac_space.h"

namespace Tac { struct Errors; }
namespace Tac
{

  struct Creation;
  struct DesktopApp;
  struct Texture;
  //struct UI2DDrawData;
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
    void                       ImGui( Errors& );
    void                       ImGuiWindows( Errors& );
    void                       ImGuiSaveAs();
    void                       ImGuiSaveAs( Entity*, Errors& );

    WindowHandle        mWindowHandle;
    UIRoot*                    mUIRoot               {};
    //UI2DDrawData*              mUI2DDrawData       {};
    Texture*                   mIconWindow           {};
    Texture*                   mIconClose            {};
    Texture*                   mIconMaximize         {};
    Texture*                   mIconMinimize         {};
    UIHierarchyNode*           mGameObjectButton     {};
    bool                       mAreTexturesLoaded    {};
    bool                       mAreLayoutsCreated    {};
    bool                       mCloseRequested       {};
  };

  const char* const gMainWindowName = "MainWindow";

}
