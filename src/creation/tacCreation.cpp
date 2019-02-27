#include "creation/tacCreation.h"
#include "shell/tacDesktopApp.h"
#include "common/tacRenderer.h"
#include "common/tacPreprocessor.h"
#include "common/tacAlgorithm.h"
#include "common/tacUI2D.h"
#include "common/tacOS.h"
#include "common/math/tacMath.h"
#include "common/tacFont.h"
#include "common/tacTextureAssetManager.h"
#include "common/tacColorUtil.h"
#include "space/tacGhost.h"
#include "space/tacworld.h"
#include "space/tacentity.h"

#include <iostream>
#include <functional>
#include <algorithm>


struct SaveWindowSize : public TacEvent<>::Handler
{
  virtual ~SaveWindowSize() = default;
  void HandleEvent() override
  {
    mWindowJson->operator[]( "w" ) = mDesktopWindow->mWidth;
    mWindowJson->operator[]( "h" ) = mDesktopWindow->mHeight;
    TacErrors errors;
    mSettings->Save( errors );
  }
  TacDesktopWindow* mDesktopWindow = nullptr;
  TacJson* mWindowJson = nullptr;
  TacSettings* mSettings = nullptr;
};

struct SaveWindowPosition : public TacEvent<>::Handler
{
  virtual ~SaveWindowPosition() = default;
  void HandleEvent() override
  {
    mWindowJson->operator[]( "x" ) = mDesktopWindow->mX;
    mWindowJson->operator[]( "y" ) = mDesktopWindow->mY;
    TacErrors errors;
    mSettings->Save( errors );
  }
  TacDesktopWindow* mDesktopWindow = nullptr;
  TacJson* mWindowJson = nullptr;
  TacSettings* mSettings = nullptr;
};

static v4 GetClearColor( TacShell* shell )
{
  return v4( 1, 0, 0, 1 );
  float visualStudioBackground = 45 / 255.0f;
  visualStudioBackground += 0.3f;
  return TacGetColorSchemeA( ( float )shell->mElapsedSeconds );
}


void TacCreation::Init( TacErrors& errors )
{
  mWorld = new TacWorld;

  TacString dataPath;
  TacOS::Instance->GetApplicationDataPath( dataPath, errors );
  TAC_HANDLE_ERROR( errors );

  TacOS::Instance->CreateFolderIfNotExist( dataPath, errors );
  TAC_HANDLE_ERROR( errors );

  TacShell* shell = mDesktopApp->mShell;
  TacSettings* settings = shell->mSettings;

  TacVector< TacString > settingsPaths = { "Windows" };
  auto windowsDefault = new TacJson();
  windowsDefault->mType = TacJsonType::Array;
  windowsDefault->mElements.push_back( new TacJson() );
  TacJson* windows = shell->mSettings->GetArray( nullptr, { "Windows" }, windowsDefault, errors );
  TAC_HANDLE_ERROR( errors );

  for( TacJson* windowJson : windows->mElements )
  {
    bool shouldCreate = shell->mSettings->GetBool( windowJson, { "Create" }, true, errors );
    TAC_HANDLE_ERROR( errors );

    if( !shouldCreate )
      continue;


    TacWindowParams windowParams = {};
    windowParams.mName = shell->mSettings->GetString( windowJson, { "Name" }, "unnamed window", errors );

    TacMonitor monitor;
    mDesktopApp->GetPrimaryMonitor( &monitor, errors );
    TAC_HANDLE_ERROR( errors );

    windowParams.mWidth = ( int )settings->GetNumber( windowJson, { "w" }, 800, errors );
    TAC_HANDLE_ERROR( errors );

    windowParams.mHeight = ( int )settings->GetNumber( windowJson, { "h" }, 600, errors );
    TAC_HANDLE_ERROR( errors );

    bool centered = ( int )settings->GetBool( windowJson, { "centered" }, false, errors );
    TAC_HANDLE_ERROR( errors );

    if( centered )
    {
      TacWindowParams::GetCenteredPosition(
        windowParams.mWidth,
        windowParams.mHeight,
        &windowParams.mX,
        &windowParams.mY,
        monitor );
    }
    else
    {
      windowParams.mX = ( int )settings->GetNumber( windowJson, { "x" }, 50, errors );
      TAC_HANDLE_ERROR( errors );

      windowParams.mY = ( int )settings->GetNumber( windowJson, { "y" }, 50, errors );
      TAC_HANDLE_ERROR( errors );
    }
    TAC_HANDLE_ERROR( errors );

    TacDesktopWindow* desktopWindow;
    mDesktopApp->SpawnWindow( windowParams, &desktopWindow, errors );
    TAC_HANDLE_ERROR( errors );


    auto saveWindowSize = new SaveWindowSize();
    saveWindowSize->mDesktopWindow = desktopWindow;
    saveWindowSize->mSettings = settings;
    saveWindowSize->mWindowJson = windowJson;
    desktopWindow->mOnResize.AddCallback( saveWindowSize );

    auto saveWindowPosition = new SaveWindowPosition();
    saveWindowPosition->mDesktopWindow = desktopWindow;
    saveWindowPosition->mSettings = settings;
    saveWindowPosition->mWindowJson = windowJson;
    desktopWindow->mOnMove.AddCallback( saveWindowPosition );

    //auto ui2DDrawData = new TacUI2DDrawData();
    //ui2DDrawData->mUI2DCommonData = shell->mUI2DCommonData;
    //ui2DDrawData->mRenderView = desktopWindow->mMainWindowRenderView;

    //auto uiRoot = new TacUIRoot();
    //uiRoot->mKeyboardInput = shell->mKeyboardInput;
    //uiRoot->mElapsedSeconds = &shell->mElapsedSeconds;
    //uiRoot->mUI2DDrawData = ui2DDrawData;

    //auto editorWindow = new TacEditorWindow();
    //editorWindow->mCreation = this;
    //editorWindow->mDesktopWindow = desktopWindow;
    //editorWindow->mUI2DDrawData = ui2DDrawData;
    //editorWindow->mUIRoot = uiRoot;

    if( windowParams.mName == gMainWindowName )
    {
      mMainWindow = new TacCreationMainWindow();
      mMainWindow->mCreation = this;
      mMainWindow->mDesktopWindow = desktopWindow;
      mMainWindow->mDesktopApp = mDesktopApp;
      mMainWindow->Init( errors );
      TAC_HANDLE_ERROR( errors );

      desktopWindow->mOnDestroyed.AddCallback( new TacFunctionalHandler( [&]()
      {
        TacOS::Instance->mShouldStopRunning = true;
        delete mMainWindow;
        mMainWindow = nullptr;
      } ) );
    }

    if( windowParams.mName == gGameWindowName )
    {
      TacAssert( !mGameWindow );
      mGameWindow = new TacCreationGameWindow();
      mGameWindow->mShell = shell;
      mGameWindow->mDesktopWindow = desktopWindow;
      mGameWindow->Init( errors );
      TAC_HANDLE_ERROR( errors );

      desktopWindow->mOnDestroyed.AddCallback( new TacFunctionalHandler( [&]()
      {
        delete mGameWindow;
        mGameWindow = nullptr;
      } ) );
    }

    if( windowParams.mName == gPropertyWindowName )
    {
      mPropertyWindow = new TacCreationPropertyWindow;
      mPropertyWindow->mShell = shell;
      mPropertyWindow->mCreation = this;
      mPropertyWindow->mDesktopWindow = desktopWindow;
      mPropertyWindow->Init( errors );
      TAC_HANDLE_ERROR( errors );

      desktopWindow->mOnDestroyed.AddCallback( new TacFunctionalHandler( [&]()
      {
        delete mPropertyWindow;
        mPropertyWindow = nullptr;
      } ) );
    }

    //mEditorWindows.insert( editorWindow );
  }

}
void TacCreation::Update( TacErrors& errors )
{
  if( mMainWindow )
  {
    mMainWindow->Update( errors );
    TAC_HANDLE_ERROR( errors );
  }

  if( mGameWindow )
  {
    mGameWindow->Update( errors );
    TAC_HANDLE_ERROR( errors );
  }

  if( mPropertyWindow )
  {
    mPropertyWindow->Update( errors );
    TAC_HANDLE_ERROR( errors );
  }
}


void TacCreation::CreateEntity()
{
  TacWorld* world = mWorld;
  TacString desiredEntityName = "Entity";
  int parenNumber = 1;
  for( ;; )
  {
    bool isEntityNameUnique = false;
    TacEntity* entity = world->FindEntity( desiredEntityName );
    if( !entity )
      break;
    desiredEntityName = "Entity (" + TacToString( parenNumber ) + ")";
    parenNumber++;
  }

  TacEntity* entity = world->SpawnEntity( TacNullEntityUUID );
  entity->mName = desiredEntityName;
  mSelectedEntity = entity;
}


void TacDesktopApp::DoStuff( TacDesktopApp* desktopApp, TacErrors& errors )
{
  TacString appDataPath;
  TacOS::Instance->GetApplicationDataPath( appDataPath, errors );

  TacString appName = "Creation";
  TacString studioPath = appDataPath + "\\Sleeping Studio\\";
  TacString prefPath = studioPath + appName;

  bool appDataPathExists;
  TacOS::Instance->DoesFolderExist( appDataPath, appDataPathExists, errors );
  TacAssert( appDataPathExists );

  TacOS::Instance->CreateFolderIfNotExist( studioPath, errors );
  TAC_HANDLE_ERROR( errors );

  TacOS::Instance->CreateFolderIfNotExist( prefPath, errors );
  TAC_HANDLE_ERROR( errors );


  TacShell* shell = desktopApp->mShell;
  shell->mAppName = appName;
  shell->mPrefPath = prefPath;
  shell->SetScopedGlobals();
  shell->Init( errors );
  TAC_HANDLE_ERROR( errors );

  desktopApp->OnShellInit( errors );
  TAC_HANDLE_ERROR( errors );


  struct TacCreationUpdater : public TacEvent<>::Handler
  {
    virtual ~TacCreationUpdater() = default;
    void HandleEvent() override { f(); }
    std::function< void() > f;
  };


  // should this really be on the heap?
  auto creation = new TacCreation();
  creation->mDesktopApp = desktopApp;

  auto creationUpdater = new TacCreationUpdater;
  creationUpdater->f = [ & ]() { creation->Update( errors ); };

  shell->mOnUpdate.AddCallback( creationUpdater );

  creation->Init( errors );
  TAC_HANDLE_ERROR( errors );

  desktopApp->Loop( errors );
  TAC_HANDLE_ERROR( errors );

  delete creation;
}


//void TacEditorWindow::Update( TacErrors& errors )
//{
//  auto uiRoot = mUIRoot;
//  TacDesktopWindow* desktopWindow = mDesktopWindow;
//  TacUI2DDrawData* uI2DDrawData = mUI2DDrawData;
//
//
//  mUIRoot->Update();
//  mUIRoot->Render( errors );
//  TAC_HANDLE_ERROR( errors );
//
//  // ^
//  // |
//  // ui renders onto the draw data so have it first
//  // otherwise, if you resize the window it will try to
//  // draw using a deleted texture
//  // |
//  // v
//
//  uI2DDrawData->DrawToTexture( errors );
//  TAC_HANDLE_ERROR( errors );
//}
