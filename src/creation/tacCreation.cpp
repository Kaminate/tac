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

#include <iostream>
#include <functional>
#include <algorithm>


const TacString gGameWindowName = "VirtualGamePlayer";

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
  TacDesktopWindow* desktopWindow = mEditorWindow->mDesktopWindow;
  mHandleMainWindowClosed = new TacHandleMainWindowClosed();
  desktopWindow->mOnDestroyed.AddCallback( mHandleMainWindowClosed );

  mDrawTitleBar = false;
  mDrawMenuBar = false;
}
void TacCreationMainWindow::Update( TacErrors& errors )
{
  TacUIRoot* uiRoot = mEditorWindow->mUIRoot;
  TacCreation* creation = mEditorWindow->mCreation;

  if( creation->mAreTexturesLoaded && !mAreLayoutsCreated )
  {
    float size = 35;

    bool experimental = true;
    if( experimental )
    {
      TacUIHierarchyNode* node = nullptr;
      TacUIHierarchyVisualImage* image = nullptr;
      TacUIHierarchyVisualText* text = nullptr;


      TacUIHierarchyNode* contentArea = uiRoot->mHierarchyRoot;

      TacUIHierarchyNode* menuBar = contentArea->Split(
        TacUISplit::Before, TacUILayoutType::Vertical );

      TacUIHierarchyNode* topBar = contentArea->Split(
        TacUISplit::Before, TacUILayoutType::Vertical );

      TacUIHierarchyNode* statusBar = contentArea->Split(
        TacUISplit::After, TacUILayoutType::Vertical );

      v4 textColor = v4( v3( 1, 1, 1 ) * 0.0f, 1 );

      if( topBar )
      {
        topBar->mDebugName = "top bar";
        topBar->mSize.y = size;

        image = new TacUIHierarchyVisualImage();
        image->mTexture = creation->mIconMinimize;
        image->mDims = { size, size };
        node = topBar->Split();
        node->SetVisual( image );

        image = new TacUIHierarchyVisualImage();
        image->mDims = { size, size };
        image->mTexture = creation->mIconMaximize;
        node = topBar->Split( TacUISplit::After, TacUILayoutType::Horizontal );
        node->SetVisual( image );

        image = new TacUIHierarchyVisualImage();
        image->mDims = { size, size };
        image->mTexture = creation->mIconClose;
        node = topBar->Split( TacUISplit::After, TacUILayoutType::Horizontal );
        node->SetVisual( image );

        text = new TacUIHierarchyVisualText();
        text->mUITextData.mUtf8 = "Gravestory (Running) - Moachers Creation Studio";
        text->mUITextData.mFontSize = 16;
        text->mUITextData.mColor = textColor;
        text->mDims = { 400, 50 };
        node = topBar->Split( TacUISplit::Before, TacUILayoutType::Horizontal );
        node->SetVisual( text );

        image = new TacUIHierarchyVisualImage();
        image->mDims = { size, size };
        image->mTexture = creation->mIconWindow;
        node = topBar->Split( TacUISplit::Before, TacUILayoutType::Horizontal );
        node->SetVisual( image );

      }

      if( menuBar )
      {
        menuBar->mDebugName = "menu bar";
        menuBar->mSize.y = 300;

        text = new TacUIHierarchyVisualText();
        text->mUITextData.mUtf8 = "Help";
        text->mUITextData.mFontSize = 16;
        text->mUITextData.mColor = textColor;
        text->mDims = { 100, 50 };
        node = menuBar->Split( TacUISplit::Before );
        node->SetVisual( text );

        text = new TacUIHierarchyVisualText();
        text->mUITextData.mUtf8 = "Window";
        text->mUITextData.mFontSize = 16;
        text->mUITextData.mColor = textColor;
        text->mDims = { 100, 50 };
        node = menuBar->Split( TacUISplit::Before );
        node->SetVisual( text );



        text = new TacUIHierarchyVisualText();
        text->mUITextData.mUtf8 = "Edit";
        text->mUITextData.mFontSize = 16;
        text->mUITextData.mColor = textColor;
        text->mDims = { 100, 50 };
        node = menuBar->Split( TacUISplit::Before );
        node->SetVisual( text );


        text = new TacUIHierarchyVisualText();
        text->mUITextData.mUtf8 = "File";
        text->mUITextData.mFontSize = 16;
        text->mUITextData.mColor = textColor;
        text->mDims = { 100, 50 };
        node = menuBar->Split( TacUISplit::Before );
        node->SetVisual( text );
      }

      if( contentArea )
      {
        contentArea->mDebugName = "content area";
      }

      if( statusBar )
      {
        statusBar->mDebugName = "status bar";
        statusBar->mSize.y = 30;
      }

      if( false )
      {
        TacString stringified = uiRoot->DebugGenerateGraphVizDotFile();
        TacString filepath = creation->mShell->mPrefPath + "/tac.dot";
        TacErrors errors;
        TacOS::Instance->SaveToFile( filepath, stringified.data(), stringified.size(), errors );
        TacAssert( errors.empty() );
      }
    }


    bool mShouldCreateTopMostBar = false;
    if( mShouldCreateTopMostBar )
    {
      mTopMostBar = uiRoot->AddMenu( "topmost bar" );
      mTopMostBar->mAutoWidth = true;
      mTopMostBar->mHeightTarget = size;

      bool mShouldCreateTopMostBarLeft = false;
      if( mShouldCreateTopMostBarLeft )
      {
        mTopMostBarLeft = mTopMostBar->Add< TacUILayout >( "topmost bar left" );
        mTopMostBarLeft->mAutoWidth = true;

        bool mShouldCreateWindowIcon = true;
        if( mShouldCreateWindowIcon )
        {
          mLayoutIconWindow = mTopMostBarLeft->Add< TacUILayout >( "window icon" );
          mLayoutIconWindow->mUiWidth = size;
          mLayoutIconWindow->mHeightTarget = size;
          mLayoutIconWindow->mTexture = creation->mIconWindow;
        }

        bool mShouldCreateTitleText = true;
        if( mShouldCreateTitleText )
        {
          TacUITextData uiTextData;
          uiTextData.mUtf8 = "Creation (Running) - Tac Studio";
          uiTextData.mFontSize = 16;
          uiTextData.mColor = v4(
            1, 0, 0,
            //90 / 255.0f,
            //111 / 255.0f,
            //102 / 255.0f,
            1 );
          mTitleText = mTopMostBarLeft->Add< TacUIText >( "creation title text" );
          mTitleText->SetText( uiTextData );
        }
      }

      bool mShouldCreateTopMostBarRight = true;
      if( mShouldCreateTopMostBarRight )
      {
        mTopMostBarRight = mTopMostBar->Add< TacUILayout >( "topmost bar right" );
        mTopMostBarRight->mAutoWidth = true;
        mTopMostBarRight->mAnchor.mAnchorHorizontal = TacUIAnchorHorizontal::Right;

        bool mShouldCreateMinimizeButton = false;
        if( mShouldCreateMinimizeButton )
        {
          mLayoutIconMinimize = mTopMostBarRight->Add< TacUILayout >( "icon minimize" );
          mLayoutIconMinimize->mTexture = creation->mIconClose;
          mLayoutIconMinimize->mUiWidth = size;
          mLayoutIconMinimize->mHeightTarget = size;
          mLayoutIconMinimize->mAnchor.mAnchorHorizontal = TacUIAnchorHorizontal::Right;
          mLayoutIconMinimize->mAnchor.mAnchorVertical = TacUIAnchorVertical::Top;
        }

        bool mShouldCreateMaximizeButton = false;
        if( mShouldCreateMaximizeButton )
        {
          mLayoutIconMaximize = mTopMostBarRight->Add< TacUILayout >( "icon maximize" );
          mLayoutIconMaximize->mTexture = creation->mIconClose;
          mLayoutIconMaximize->mUiWidth = size;
          mLayoutIconMaximize->mHeightTarget = size;
          mLayoutIconMaximize->mAnchor.mAnchorHorizontal = TacUIAnchorHorizontal::Right;
          mLayoutIconMaximize->mAnchor.mAnchorVertical = TacUIAnchorVertical::Top;
        }

        bool mShouldCreateCloseIcon = true;
        if( mShouldCreateCloseIcon )
        {
          mLayoutIconClose = mTopMostBarRight->Add< TacUILayout >( "icon close" );
          mLayoutIconClose->mTexture = creation->mIconClose;
          mLayoutIconClose->mUiWidth = size;
          mLayoutIconClose->mHeightTarget = size;
          mLayoutIconClose->mAnchor.mAnchorHorizontal = TacUIAnchorHorizontal::Right;
          mLayoutIconClose->mAnchor.mAnchorVertical = TacUIAnchorVertical::Top;
        }
      }
    }

    mAreLayoutsCreated = true;
  }
  TacDesktopWindow* desktopWindow = mEditorWindow->mDesktopWindow;
  TacUI2DDrawData* uI2DDrawData = mEditorWindow->mUI2DDrawData;
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
          creation->mIconWindow );
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
          creation->mIconClose );
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
    mApp->SpawnWindowOuter( windowParams, &desktopWindow, errors );
    TAC_HANDLE_ERROR( errors );


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

    auto ui2DDrawData = new TacUI2DDrawData();
    ui2DDrawData->mUI2DCommonData = shell->mUI2DCommonData;
    ui2DDrawData->mRenderView = desktopWindow->mMainWindowRenderView;

    auto uiRoot = new TacUIRoot();
    uiRoot->mKeyboardInput = shell->mKeyboardInput;
    uiRoot->mElapsedSeconds = &shell->mElapsedSeconds;
    uiRoot->mUI2DDrawData = ui2DDrawData;

    auto editorWindow = new TacEditorWindow();
    editorWindow->mCreation = this;
    editorWindow->mDesktopWindow = desktopWindow;
    editorWindow->mUI2DDrawData = ui2DDrawData;
    editorWindow->mUIRoot = uiRoot;

    if( !mainWindow )
    {
      mainWindow = desktopWindow;
      mMainWindow = new TacCreationMainWindow();
      mMainWindow->mEditorWindow = editorWindow;
      mMainWindow->Init( errors );
      TAC_HANDLE_ERROR( errors );
    }

    if( windowParams.mName == gGameWindowName )
    {
      TacAssert( !mGameWindow );
      mGameWindow = new TacCreationGameWindow();
      mGameWindow->mEditorWindow = editorWindow;
      mGameWindow->Init( errors );
      TAC_HANDLE_ERROR( errors );
    }

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
        loadedTextureCount++;
    }
    if( loadedTextureCount == textureAndPaths.size() )
      mAreTexturesLoaded = true;
  }

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
  creation->mApp = desktopApp;
  creation->mShell = shell;

  auto creationUpdater = new TacCreationUpdater;
  creationUpdater->f = [ & ]() { creation->Update( errors ); };

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


  mUIRoot->Update();
  mUIRoot->Render( errors );
  TAC_HANDLE_ERROR( errors );

  // ^
  // |
  // ui renders onto the draw data so have it first
  // otherwise, if you resize the window it will try to
  // draw using a deleted texture
  // |
  // v

  uI2DDrawData->DrawToTexture( errors );
  TAC_HANDLE_ERROR( errors );
}

void TacCreationGameWindow::Init( TacErrors& errors)
{
  TacShell* shell = mEditorWindow->mCreation->mShell;

  mRenderView = new TacRenderView();

  auto uI2DDrawData = new TacUI2DDrawData();
  uI2DDrawData->mUI2DCommonData = shell->mUI2DCommonData;
  uI2DDrawData->mRenderView = mRenderView;

  auto ghost = new TacGhost( shell, errors);
  TAC_HANDLE_ERROR( errors );
  ghost->mUIRoot->mUI2DDrawData = uI2DDrawData;
  ghost->mRenderView = mRenderView;

  ghost->Init( errors );
  TAC_HANDLE_ERROR( errors );

  shell->AddSoul( ghost );
  mSoul = ghost;

  struct TacGameVis : public TacUIHierarchyVisual
  {
    void Render( TacErrors& errors ) override
    {
      TacTexture* texture;
      TacString path;
      TacRenderer* renderer = mRenderer;
      
      path = "assets/vgb_blue_big.png";
      mTextureAssetManager->GetTexture( &texture, path, errors );
      TacUI2DState& state = mHierarchyNode->mUIRoot->mUI2DDrawData->mStates.back();
      state.Draw2DBox(
        ( float )mDesktopWindow->mWidth,
        ( float )mDesktopWindow->mHeight,
        v4( 1, 1, 1, 1 ),
        texture );

      float innerBoxPercentOffsetX = 0.159f;
      float innerBoxPercentOffsetY = 0.175f;
      float innerBoxPercentWidth = 1 - ( 2 * innerBoxPercentOffsetX );
      float innerBoxPercentHeight = 1 - ( 2 * innerBoxPercentOffsetY );

      float innerBoxPixelOffsetY = innerBoxPercentOffsetX * ( float )mDesktopWindow->mWidth;
      float innerBoxPixelOffsetX = innerBoxPercentOffsetY * ( float )mDesktopWindow->mHeight;
      float innerBoxPixelWidth = innerBoxPercentWidth * ( float )mDesktopWindow->mWidth;
      float innerBoxPixelHeight = innerBoxPercentHeight * ( float )mDesktopWindow->mHeight;
      state.Translate( innerBoxPixelOffsetY, innerBoxPixelOffsetX );

      TacTexture* outputColor = mRenderView->mFramebuffer;
      TacDepthBuffer* outputDepth = mRenderView->mFramebufferDepth;
      if( !outputColor || outputColor->myImage.mWidth != ( int )innerBoxPixelWidth )
      {
        if( outputColor )
        {
          mRenderer->RemoveRendererResource( outputColor );
          mRenderer->RemoveRendererResource( outputDepth );
          outputColor = nullptr;
          outputDepth = nullptr;
        }

        TacImage image;
        image.mWidth = ( int )innerBoxPixelWidth;
        image.mHeight = ( int )innerBoxPixelHeight;
        image.mFormat.mPerElementByteCount = 1;
        image.mFormat.mElementCount = 4;
        image.mFormat.mPerElementDataType = TacGraphicsType::unorm;
        TacTextureData textureData;
        textureData.access = TacAccess::Default;
        textureData.binding = { TacBinding::RenderTarget, TacBinding::ShaderResource };
        textureData.cpuAccess = {};
        textureData.mName = "client view fbo";
        textureData.mStackFrame = TAC_STACK_FRAME;
        textureData.myImage = image;
        renderer->AddTextureResource( &outputColor, textureData, errors );
        TAC_HANDLE_ERROR( errors );

        TacDepthBufferData depthBufferData;
        depthBufferData.mName = "client view depth buffer";
        depthBufferData.mStackFrame = TAC_STACK_FRAME;
        depthBufferData.width = ( int )innerBoxPixelWidth;
        depthBufferData.height  = ( int )innerBoxPixelHeight;
        renderer->AddDepthBuffer( &outputDepth, depthBufferData, errors );
        TAC_HANDLE_ERROR( errors );

        mRenderView->mFramebuffer = outputColor;
        mRenderView->mFramebufferDepth = outputDepth;
      }

      //path = "assets/unknown.png";
      //mTextureAssetManager->GetTexture( &texture, path, errors );
      texture = outputColor;

      state.Draw2DBox(
        innerBoxPixelWidth,
        innerBoxPixelHeight,
        v4( 1, 1, 1, 1 ),
        texture );

    }
    v2 GetSize() override
    {
      return
      {
        ( float )mDesktopWindow->mWidth,
        ( float )mDesktopWindow->mHeight
      };
    }
    TacString GetDebugName() override
    {
      return "game vis";
    }
    TacDesktopWindow* mDesktopWindow = nullptr;
    TacTextureAssetManager* mTextureAssetManager = nullptr;
    TacRenderer* mRenderer = nullptr;
    TacSoul* mSoul = nullptr;
    TacRenderView* mRenderView = nullptr;

  };

  auto gameVis = new TacGameVis();
  gameVis->mDesktopWindow = mEditorWindow->mDesktopWindow;
  gameVis->mTextureAssetManager = mEditorWindow->mCreation->mTextureAssetManager;
  gameVis->mRenderer = mEditorWindow->mCreation->mShell->mRenderer;
  gameVis->mSoul = ghost;
  gameVis->mRenderView = mRenderView;


  mEditorWindow->mUIRoot->mHierarchyRoot->SetVisual( gameVis);
}
void TacCreationGameWindow::Update( TacErrors& errors )
{
}
