#pragma once


struct TacUILayout;
struct TacShell;
struct TacUIText;
struct TacDesktopWindow;
struct TacUIRoot;
struct TacUI2DDrawData;
struct TacErrors;
struct TacTexture;


struct TacCreationMainWindow
{
  void Init( TacErrors& errors );
  void Update( TacErrors& errors );
  void LoadTextures( TacErrors& errors );
  void CreateLayouts();

  TacShell* mShell = nullptr;
  TacDesktopWindow* mDesktopWindow = nullptr;
  TacUIRoot* mUIRoot = nullptr;
  TacUI2DDrawData* mUI2DDrawData = nullptr;
  TacTexture* mIconWindow = nullptr;
  TacTexture* mIconClose = nullptr;
  TacTexture* mIconMaximize = nullptr;
  TacTexture* mIconMinimize = nullptr;
  bool mAreTexturesLoaded = false;
  bool mAreLayoutsCreated = false;
};
