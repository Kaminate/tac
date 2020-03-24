#pragma once

#include "common/tacErrorHandling.h"

struct TacUILayout;
struct TacShell;
struct TacUIText;
struct TacDesktopWindow;
struct TacDesktopApp;
struct TacCreation;
struct TacUIRoot;
struct TacUI2DDrawData;
struct TacUIHierarchyNode;
struct TacTexture;
struct TacCreationGameObjectMenuWindow;

struct TacCreationMainWindow
{
  ~TacCreationMainWindow();
  void Init( TacErrors& errors );
  void Update( TacErrors& errors );
  void LoadTextures( TacErrors& errors );
  void ImGui();
  void ImGuiWindows();

  TacCreation* mCreation = nullptr;
  TacCreationGameObjectMenuWindow* mGameObjectMenuWindow = nullptr;

  TacDesktopWindow* mDesktopWindow = nullptr;
  TacUIRoot* mUIRoot = nullptr;
  TacUI2DDrawData* mUI2DDrawData = nullptr;
  TacTexture* mIconWindow = nullptr;
  TacTexture* mIconClose = nullptr;
  TacTexture* mIconMaximize = nullptr;
  TacTexture* mIconMinimize = nullptr;
  TacUIHierarchyNode* mGameObjectButton = nullptr;
  bool mAreTexturesLoaded = false;
  bool mAreLayoutsCreated = false;
  TacErrors mButtonCallbackErrors;
};

const TacString gMainWindowName = "MainWindow";
