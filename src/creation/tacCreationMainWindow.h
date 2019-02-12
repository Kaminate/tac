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
struct TacCreationMainWindow;

struct TacCreationGameObjectMenuWindow
{
  ~TacCreationGameObjectMenuWindow();
  void Init( TacErrors& errors );
  void CreateLayouts();
  void Update( TacErrors& errors );
  TacCreationMainWindow* mMainWindow = nullptr;
  TacDesktopWindow* mDesktopWindow = nullptr;
  TacUIRoot* mUIRoot = nullptr;
  TacUI2DDrawData* mUI2DDrawData = nullptr;
  TacShell* mShell = nullptr;
  double mCreationSeconds;
};

struct TacCreationMainWindow
{
  void Init( TacErrors& errors );
  void Update( TacErrors& errors );
  void LoadTextures( TacErrors& errors );
  void CreateLayouts();

  TacCreation* mCreation = nullptr;
  TacShell* mShell = nullptr;
  TacDesktopWindow* mDesktopWindow = nullptr;
  TacDesktopApp* mDesktopApp = nullptr;
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
  TacCreationGameObjectMenuWindow* mGameObjectMenuWindow = nullptr;
};
