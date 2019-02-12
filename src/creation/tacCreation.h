#pragma once

#include "common/tacMemory.h"
#include "common/tacErrorHandling.h"
#include "common/tacRenderer.h"
#include "common/tacSettings.h"
#include "common/tacEvent.h"
#include "common/tacUI.h"
#include "common/containers/tacVector.h"
#include "creation/tacCreationGameWindow.h"
#include "creation/tacCreationPropertyWindow.h"
#include "creation/tacCreationMainWindow.h"

struct TacDesktopWindow;
struct TacDesktopApp;
struct TacRenderer;
struct TacUI2DDrawData;
struct TacCreation;
struct TacShell;
struct TacSoul;
struct TacWindowParams;
struct TacTextureAssetManager;
struct TacTexture;



//struct TacEditorWindow
//{
//  void Update( TacErrors& errors );
//  TacDesktopWindow* mDesktopWindow = nullptr;
//  TacCreation* mCreation = nullptr;
//  TacUIRoot* mUIRoot = nullptr;
//  TacUI2DDrawData* mUI2DDrawData = nullptr;
//};

struct TacCreation
{
  void Init( TacErrors& errors );
  void Update( TacErrors& errors );

  //void UpdateGamePlayer( TacErrors& );
  //void UpdatePropertiesWindow( TacErrors& );

  TacDesktopApp* mApp = nullptr;

  TacCreationMainWindow* mMainWindow = nullptr;
  TacCreationGameWindow* mGameWindow = nullptr;
  TacCreationPropertyWindow* mPropertyWindow = nullptr;

  //TacEditorWindow* mSecondWindow = nullptr;
  //TacEditorWindow* mGamePlayerWindow = nullptr;
  TacShell* mShell = nullptr;

  TacTextureAssetManager* mTextureAssetManager = nullptr;
  //std::set< TacEditorWindow* > mEditorWindows;
};

const v4 textColor = v4( v3( 1, 1, 1 ) * 0.0f, 1 );
