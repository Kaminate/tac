#pragma once

#include "common/tacMemory.h"
#include "common/tacErrorHandling.h"
#include "common/tacRenderer.h"
#include "common/tacSettings.h"
#include "common/tacEvent.h"
#include "common/tacUI.h"
#include "common/containers/tacVector.h"

struct TacDesktopWindow;
struct TacDesktopApp;
struct TacRenderer;
struct TacUI2DDrawData;
struct TacCreation;
struct TacShell;
struct TacWindowParams;
struct TacTextureAssetManager;
struct TacTexture;


struct TacHandleMainWindowClosed : public TacEvent<>::Handler
{
  void HandleEvent() override;
};

struct TacEditorWindow
{
  void Update( TacErrors& errors );
  TacDesktopWindow* mDesktopWindow = nullptr;
  TacCreation* mCreation = nullptr;
  TacUIRoot* mUIRoot = nullptr;
  TacUI2DDrawData* mUI2DDrawData = nullptr;
};

struct TacCreationMainWindow
{
  void Init( TacErrors& errors );
  void Update( TacErrors& errors );
  TacOwned< TacHandleMainWindowClosed > mHandleMainWindowClosed;

  TacUILayout* mTopMostBar = nullptr;
  TacUILayout* mTopMostBarLeft = nullptr;
  TacUILayout* mTopMostBarRight = nullptr;
  TacUILayout* mLayoutIconWindow = nullptr;
  TacUILayout* mLayoutIconClose = nullptr;
  TacUILayout* mLayoutIconMaximize = nullptr;
  TacUILayout* mLayoutIconMinimize = nullptr;
  TacUIText* mTitleText = nullptr;
  bool mAreLayoutsCreated = false;

  bool mDrawTitleBar;
  bool mDrawMenuBar;
  TacEditorWindow* mEditorWindow = nullptr;
};

struct TacCreation
{
  void Init( TacErrors& errors );
  void Update( TacErrors& errors );

  //void UpdateGamePlayer( TacErrors& );
  //void UpdatePropertiesWindow( TacErrors& );

  TacDesktopApp* mApp = nullptr;
  TacCreationMainWindow* mMainWindow = nullptr;
  //TacEditorWindow* mSecondWindow = nullptr;
  //TacEditorWindow* mGamePlayerWindow = nullptr;
  TacShell* mShell = nullptr;

  TacTextureAssetManager* mTextureAssetManager = nullptr;

  // These textures shouldnt really, be here
  // they should be prerequisites of each window.
  TacTexture* mIconWindow = nullptr;
  TacTexture* mIconClose = nullptr;
  TacTexture* mIconMaximize = nullptr;
  TacTexture* mIconMinimize = nullptr;
  bool mAreTexturesLoaded = false;

  std::set< TacEditorWindow* > mEditorWindows;
};

