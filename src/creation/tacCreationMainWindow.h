
#pragma once

#include "src/common/tacErrorHandling.h"

namespace Tac
{
struct UILayout;
struct Shell;
struct UIText;
struct DesktopWindow;
struct DesktopApp;
struct Creation;
struct UIRoot;
struct UI2DDrawData;
struct UIHierarchyNode;
struct Texture;
struct CreationGameObjectMenuWindow;

struct CreationMainWindow
{
  ~CreationMainWindow();
  void Init( Errors& errors );
  void Update( Errors& errors );
  void LoadTextures( Errors& errors );
  void ImGui();
  void ImGuiWindows();

  Creation* mCreation = nullptr;
  CreationGameObjectMenuWindow* mGameObjectMenuWindow = nullptr;

  DesktopWindow* mDesktopWindow = nullptr;
  UIRoot* mUIRoot = nullptr;
  UI2DDrawData* mUI2DDrawData = nullptr;
  Texture* mIconWindow = nullptr;
  Texture* mIconClose = nullptr;
  Texture* mIconMaximize = nullptr;
  Texture* mIconMinimize = nullptr;
  UIHierarchyNode* mGameObjectButton = nullptr;
  bool mAreTexturesLoaded = false;
  bool mAreLayoutsCreated = false;
  Errors mButtonCallbackErrors;
};

const String gMainWindowName = "MainWindow";

}

