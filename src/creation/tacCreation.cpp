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

#include <iostream>
#include <functional>
#include <algorithm>


struct SaveWindowSize : public TacEvent<>::Handler
{
  virtual ~SaveWindowSize() = default;
  void HandleEvent() override
  {
    mWindowJson->operator[]( "w" ) = mDesktopWindow->mWidth;
    mWindowJson->operator[]( "y" ) = mDesktopWindow->mHeight;
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
  float visualStudioBackground = 45 / 255.0f;
  visualStudioBackground += 0.3f;
  return v4( v3( 1, 1, 1 ) * visualStudioBackground, 1.0f );
  v4 clearColorRGBA;
  v3 a = { 0.8f, 0.5f, 0.4f };
  v3 b = { 0.2f, 0.4f, 0.2f };
  v3 c = { 2.0f, 1.0f, 1.0f };
  v3 d = { 0.0f, 0.25f, 0.25f };
  for( int i = 0; i < 3; ++i )
  {
    float v = c[ i ];
    v *= ( float )shell->mElapsedSeconds;
    v *= 0.15f;
    v += d[ i ];
    v *= 2.0f;
    v *= 3.14f;
    v = std::cos( v );
    v *= b[ i ];
    v += a[ i ];
    clearColorRGBA[ i ] = v;
  }
  float scale = 0.2f;
  clearColorRGBA[ 0 ] *= scale;
  clearColorRGBA[ 1 ] *= scale;
  clearColorRGBA[ 2 ] *= scale;
  clearColorRGBA[ 3 ] = 1.0f;
  return clearColorRGBA;
}

static void SetRenderViewDefaults( TacShell* shell, TacDesktopWindow* desktopWindow )
{
  if( !desktopWindow )
    return;
  TacTexture* currentBackbufferTexture = nullptr;
  desktopWindow->mRendererData->GetCurrentBackbufferTexture( &currentBackbufferTexture );
  TacAssert( currentBackbufferTexture );


  TacRenderView* mMainWindowRenderView = desktopWindow->mMainWindowRenderView;
  {
    TacScissorRect scissorRect;
    scissorRect.mXMaxRelUpperLeftCornerPixel = ( float )currentBackbufferTexture->myImage.mWidth;
    scissorRect.mYMaxRelUpperLeftCornerPixel = ( float )currentBackbufferTexture->myImage.mHeight;

    TacViewport viewport;
    viewport.mViewportPixelWidthIncreasingRight = ( float )currentBackbufferTexture->myImage.mWidth;
    viewport.mViewportPixelHeightIncreasingUp = ( float )currentBackbufferTexture->myImage.mHeight;

    mMainWindowRenderView->mFramebuffer = currentBackbufferTexture;
    mMainWindowRenderView->mFramebufferDepth = desktopWindow->mRendererData->mDepthBuffer;
    mMainWindowRenderView->mClearColorRGBA = GetClearColor( shell );
    mMainWindowRenderView->mScissorRect = scissorRect;
    mMainWindowRenderView->mViewportRect = viewport;
  }
}

void TacHandleMainWindowClosed::HandleEvent()
{
  TacOS::Instance->mShouldStopRunning = true;
}

void TacCreationMainWindow::Init( TacErrors& errors )
{
  TacWindowParams mainWindowParams;
  mainWindowParams.mName = "MainWindow";

  mWindowParams = &mainWindowParams;

  TAC_HANDLE_ERROR( errors );

  mHandleMainWindowClosed = new TacHandleMainWindowClosed();
  mDesktopWindow->mOnDestroyed.AddCallback( mHandleMainWindowClosed );

  mDrawTitleBar = false;
  mDrawMenuBar = false;

  mShouldCreateTopMostBar = false;
  mShouldCreateTopMostBarLeft = false;
  mShouldCreateTopMostBarRight = false;
  mShouldCreateWindowIcon = false;
  mShouldCreateCloseIcon = false;
  mShouldCreateMaximizeButton = false;
  mShouldCreateMinimizeButton = false;
  mShouldCreateTitleText = false;
}
void TacCreationMainWindow::Update( TacErrors& errors )
{
  auto uiRoot = mUIRoot;

  if( mCreation->mAreTexturesLoaded && !mAreLayoutsCreated )
  {
    bool EXPERIMENTAL_UI = true;
    if( EXPERIMENTAL_UI )
    {
      //uiRoot->mUIE->SplitVertically();
    }







    float size = 35;

    if( mShouldCreateTopMostBar )
    {
      mTopMostBar = uiRoot->AddMenu( "topmost bar" );
      mTopMostBar->mAutoWidth = true;
      mTopMostBar->mHeightTarget = size;

      if( mShouldCreateTopMostBarLeft )
      {
        mTopMostBarLeft = mTopMostBar->Add< TacUILayout >( "topmost bar left" );
        mTopMostBarLeft->mAutoWidth = true;

        if( mShouldCreateWindowIcon )
        {
          mLayoutIconWindow = mTopMostBarLeft->Add< TacUILayout >( "window icon" );
          mLayoutIconWindow->mUiWidth = size;
          mLayoutIconWindow->mHeightTarget = size;
          mLayoutIconWindow->mTexture = mCreation->mIconWindow;
        }

        if( mShouldCreateTitleText )
        {
          TacUITextData uiTextData;
          uiTextData.mUtf8 = "Creation (Running) - Tac Studio";
          uiTextData.mFontSize = 16;
          uiTextData.mColor = v4(
            90 / 255.0f,
            111 / 255.0f,
            102 / 255.0f,
            1 );
          mTitleText = mTopMostBarLeft->Add< TacUIText >( "creation title text" );
          mTitleText->SetText( uiTextData );
        }
      }

      if( mShouldCreateTopMostBarRight )
      {
        mTopMostBarRight = mTopMostBar->Add< TacUILayout >( "topmost bar right" );
        mTopMostBarRight->mAutoWidth = true;
        mTopMostBarRight->mAnchor.mAnchorHorizontal = TacUIAnchorHorizontal::Right;

        if( mShouldCreateMinimizeButton )
        {
          mLayoutIconMinimize = mTopMostBarRight->Add< TacUILayout >( "icon minimize" );
          mLayoutIconMinimize->mTexture = mCreation->mIconClose;
          mLayoutIconMinimize->mUiWidth = size;
          mLayoutIconMinimize->mHeightTarget = size;
          mLayoutIconMinimize->mAnchor.mAnchorHorizontal = TacUIAnchorHorizontal::Right;
          mLayoutIconMinimize->mAnchor.mAnchorVertical = TacUIAnchorVertical::Top;
        }

        if( mShouldCreateMaximizeButton )
        {
          mLayoutIconMaximize = mTopMostBarRight->Add< TacUILayout >( "icon maximize" );
          mLayoutIconMaximize->mTexture = mCreation->mIconClose;
          mLayoutIconMaximize->mUiWidth = size;
          mLayoutIconMaximize->mHeightTarget = size;
          mLayoutIconMaximize->mAnchor.mAnchorHorizontal = TacUIAnchorHorizontal::Right;
          mLayoutIconMaximize->mAnchor.mAnchorVertical = TacUIAnchorVertical::Top;
        }

        if( mShouldCreateCloseIcon )
        {
          mLayoutIconClose = mTopMostBarRight->Add< TacUILayout >( "icon close" );
          mLayoutIconClose->mTexture = mCreation->mIconClose;
          mLayoutIconClose->mUiWidth = size;
          mLayoutIconClose->mHeightTarget = size;
          mLayoutIconClose->mAnchor.mAnchorHorizontal = TacUIAnchorHorizontal::Right;
          mLayoutIconClose->mAnchor.mAnchorVertical = TacUIAnchorVertical::Top;
        }
      }
    }
    mAreLayoutsCreated = true;
  }
  TacDesktopWindow* desktopWindow = mDesktopWindow;
  TacUI2DDrawData* uI2DDrawData = mUI2DDrawData;
  TacUI2DState* state = uI2DDrawData->PushState();

  TacTexture* currentBackbufferTexture = nullptr;
  desktopWindow->mRendererData->GetCurrentBackbufferTexture( &currentBackbufferTexture );
  TacAssert( currentBackbufferTexture );

  float iconW = 25;
  float iconH = 25;
  float padding = 10;

  state->Translate( padding, padding );

  // title bar
  if( mDrawTitleBar )
  {

    // Left aligned
    {
      // Icon
      {
        state->Draw2DBox(
          iconW,
          iconH,
          v4( 1, 1, 1, 1 ),
          mCreation->mIconWindow );
        state->Translate( iconW, 0 );
      }

      state->Translate( padding, 0 );

      // Title text
      state->Draw2DBox( 150, iconH, v4( 1, 1, 1, 1 ), nullptr ); // "Creation"
    }

    // Right aligned
    {

      state->mTransform = M3Translate( ( float )currentBackbufferTexture->myImage.mWidth, padding );
      {
        state->Translate( -iconW, 0 );
        state->Draw2DBox(
          iconW,
          iconH,
          v4( 1, 1, 1, 1 ),
          mCreation->mIconClose );
      }

      state->Translate( -padding, 0 );
      {
        state->Translate( -iconW, 0 );
        TacTexture* textureMaximizeWindow = nullptr;
        state->Draw2DBox(
          iconW,
          iconH,
          v4( 1, 1, 1, 1 ),
          textureMaximizeWindow );
      }
      state->Translate( -padding, 0 );

      {
        state->Translate( -iconW, 0 );
        TacTexture* textureMinimizeWindow = nullptr;
        state->Draw2DBox(
          iconW,
          iconH,
          v4( 1, 1, 1, 1 ),
          textureMinimizeWindow );
      }
    }


  }

  state->mTransform = M3Translate( padding, padding + iconH + padding );

  // Menu bar
  if( mDrawMenuBar )
  {
    TacTexture* textureFile = nullptr;
    state->Draw2DBox(
      iconW,
      iconH,
      v4( 1, 1, 1, 1 ),
      textureFile );
    state->Translate( iconW, 0 );

    state->Translate( padding, 0 );

    TacTexture* textureWindows = nullptr;
    state->Draw2DBox(
      iconW,
      iconH,
      v4( 1, 1, 1, 1 ),
      textureWindows );
    state->Translate( iconW, 0 );

    state->Translate( padding, 0 );

    TacTexture* textureHelp = nullptr;
    state->Draw2DBox(
      iconW,
      iconH,
      v4( 1, 1, 1, 1 ),
      textureHelp );
    state->Translate( iconW, 0 );
  }

  uI2DDrawData->PopState();

  TacEditorWindow::Update( errors );
  TAC_HANDLE_ERROR( errors );

}

void TacCreation::Init( TacErrors& errors )
{

  TacString dataPath;
  TacOS::Instance->GetApplicationDataPath( dataPath, errors );
  TAC_HANDLE_ERROR( errors );

  TacOS::Instance->CreateFolderIfNotExist( dataPath, errors );
  TAC_HANDLE_ERROR( errors );

  auto shell = mShell;
  mTextureAssetManager = mShell->mTextureAssetManager;

  // create ui
  {
    auto ui2DCommonData = new TacUI2DCommonData();
    ui2DCommonData->mRenderer = shell->mRenderer;
    ui2DCommonData->mFontStuff = shell->mFontStuff;
    ui2DCommonData->Init( errors );
    TAC_HANDLE_ERROR( errors );
    shell->mUI2DCommonData = ui2DCommonData;
  }

  TacSettings* settings = shell->mSettings;

  TacVector< TacString > settingsPaths = { "Windows" };
  auto windowsDefault = new TacJson();
  windowsDefault->mType = TacJsonType::Array;
  windowsDefault->mElements.push_back( new TacJson() );
  TacJson* windows = shell->mSettings->GetArray( nullptr, { "Windows" }, windowsDefault, errors );
  TAC_HANDLE_ERROR( errors );

  TacDesktopWindow* mainWindow = nullptr;

  for( TacJson* windowJson : windows->mElements )
  {
    bool shouldCreate = shell->mSettings->GetBool( windowJson, { "Create" }, true, errors );
    TAC_HANDLE_ERROR( errors );

    if( !shouldCreate )
      continue;


    TacWindowParams windowParams = {};
    windowParams.mName = shell->mSettings->GetString( windowJson, { "Name" }, "unnamed window", errors );

    TacMonitor monitor;
    mApp->GetPrimaryMonitor( &monitor, errors );
    TAC_HANDLE_ERROR( errors );

    windowParams.mWidth = ( int )settings->GetNumber( windowJson, { "w" }, 800, errors );
    TAC_HANDLE_ERROR( errors );

    windowParams.mHeight  = ( int )settings->GetNumber( windowJson, { "h" }, 600, errors );
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
      windowParams.mX = ( int )settings->GetNumber( windowJson, { "x" }, 50,  errors );
      TAC_HANDLE_ERROR( errors );

      windowParams.mY = ( int )settings->GetNumber( windowJson, { "y" }, 50,  errors );
      TAC_HANDLE_ERROR( errors );
    }
    TAC_HANDLE_ERROR( errors );

    TacDesktopWindow* desktopWindow;
    mApp->SpawnWindowOuter( windowParams, &desktopWindow, errors );
    TAC_HANDLE_ERROR( errors );

    if( !mainWindow )
      mainWindow = desktopWindow;

    {
      auto saveWindowSize = new SaveWindowSize();
      saveWindowSize->mDesktopWindow = desktopWindow;
      saveWindowSize->mSettings = mShell->mSettings;
      saveWindowSize->mWindowJson = windowJson;
      desktopWindow->mOnResize.AddCallback( saveWindowSize );
    }

    {
      auto saveWindowPosition = new SaveWindowPosition();
      saveWindowPosition->mDesktopWindow = desktopWindow;
      saveWindowPosition->mSettings = mShell->mSettings;
      saveWindowPosition->mWindowJson = windowJson;
      desktopWindow->mOnMove.AddCallback( saveWindowPosition );
    }
    TAC_HANDLE_ERROR( errors );

    TacEditorWindow* editorWindow = new TacEditorWindow();
    editorWindow->mCreation = this;
    editorWindow->mDesktopWindow = desktopWindow;

    auto ui2DDrawData = new TacUI2DDrawData();
    ui2DDrawData->mUI2DCommonData = shell->mUI2DCommonData;
    ui2DDrawData->mRenderView = desktopWindow->mMainWindowRenderView;
    editorWindow->mUI2DDrawData = ui2DDrawData;

    auto uiRoot = new TacUIRoot();
    uiRoot->mKeyboardInput = shell->mKeyboardInput;
    uiRoot->mElapsedSeconds = &shell->mElapsedSeconds;
    uiRoot->mUI2DDrawData = ui2DDrawData;
    uiRoot->mDesktopWindow = desktopWindow;
    editorWindow->mUIRoot = uiRoot;
    mEditorWindows.insert( editorWindow );
  }
}
void TacCreation::Update( TacErrors& errors )
{
  TacShell* shell = mShell;

  for( TacDesktopWindow* desktopWindow : mApp->mMainWindows )
  {
    SetRenderViewDefaults( mShell, desktopWindow );
  }

  for( TacEditorWindow* editorWindow : mEditorWindows )
  {
    editorWindow->Update( errors );
    TAC_HANDLE_ERROR( errors );
  }

  if( !mAreTexturesLoaded )
  {


    struct TacTextureAndPath
    {
      TacTexture** texture;
      const char* path;
    };
    TacVector< TacTextureAndPath > textureAndPaths = {
      { &mIconWindow, "assets/grave.png" },
    { &mIconClose, "assets/icons/close.png" },
    { &mIconMinimize, "assets/icons/minimize.png" },
    { &mIconMaximize, "assets/icons/maximize.png" },
    };
    int loadedTextureCount = 0;
    for( TacTextureAndPath textureAndPath : textureAndPaths )
    {
      mTextureAssetManager->GetTexture( textureAndPath.texture, textureAndPath.path, errors );
      TAC_HANDLE_ERROR( errors );
      if( *textureAndPath.texture )
      {
        loadedTextureCount++;
      }
    }

    if( loadedTextureCount == textureAndPaths.size() )
    {
      mAreTexturesLoaded = true;
    }
  }
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


  TacShell* shell = desktopApp->mShell ;
  shell->mAppName = appName;
  shell->mPrefPath = prefPath;
  shell->SetScopedGlobals();
  shell->Init( errors );
  TAC_HANDLE_ERROR( errors );


  struct TacCreationUpdater : public TacEvent<>::Handler
  {
    virtual ~TacCreationUpdater() = default;
    void HandleEvent() override { f(); }
    std::function< void() > f;
  };


  // should this really be on the heap?
  auto creation = new TacCreation();
  creation->mApp = desktopApp;
  creation->mShell = shell;

  auto creationUpdater = new TacCreationUpdater;
  creationUpdater->f = [ & ]() { creation->Update( errors ); };
  creation->mCreationUpdater = creationUpdater;


  shell->mOnUpdate.AddCallback( creationUpdater );

  creation->Init( errors );
  TAC_HANDLE_ERROR( errors );

  desktopApp->Loop( errors );
  TAC_HANDLE_ERROR( errors );

  delete creation;
}


void TacEditorWindow::Update( TacErrors& errors )
{
  auto uiRoot = mUIRoot;
  TacDesktopWindow* desktopWindow = mDesktopWindow;
  TacUI2DDrawData* uI2DDrawData = mUI2DDrawData;

  uI2DDrawData->DrawToTexture( errors );
  TAC_HANDLE_ERROR( errors );

  mUIRoot->Update();
  mUIRoot->Render( errors );
  TAC_HANDLE_ERROR( errors );
}
