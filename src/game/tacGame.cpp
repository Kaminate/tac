#include "game/tacGame.h"
#include "shell/tacDesktopApp.h"
#include "space/tacGhost.h"
#include "common/tacOS.h"
#include "common/tacUI2D.h"
#include "common/tacUI.h"
#include "common/tacImGui.h"

#include <functional> // std::function

TacString appName = "Gravestory";

struct TacGame
{
  void Init( TacErrors& errors )
  {
    TacMonitor monitor;
    mApp->GetPrimaryMonitor( &monitor, errors );
    TAC_HANDLE_ERROR( errors );

    TacWindowParams windowParams = {};
    windowParams.mName = appName;
    windowParams.mWidth = ( int )( 0.8f * monitor.w );
    windowParams.mHeight = ( int )( 0.8f * monitor.h );
    TacWindowParams::GetCenteredPosition(
      windowParams.mWidth,
      windowParams.mHeight,
      &windowParams.mX,
      &windowParams.mY,
      monitor );

    mApp->SpawnWindow( windowParams, &mDesktopWindow, errors );
    TAC_HANDLE_ERROR( errors );

    mUi2DDrawData = new TacUI2DDrawData;
    mUi2DDrawData->mUI2DCommonData = mShell->mUI2DCommonData;
    mUi2DDrawData->mRenderView = mDesktopWindow->mRenderView;

    auto ghost = new TacGhost;
    ghost->mShell = mShell;
    ghost->mRenderView = mDesktopWindow->mRenderView;
    //ghost->mUIRoot->mUI2DDrawData = mUi2DDrawData;
    ghost->Init( errors );
    TAC_HANDLE_ERROR( errors );
    mShell->AddSoul( ghost );
  }
  void SetImGuiGlobals()
  {
    TacErrors screenspaceCursorPosErrors;
    v2 screenspaceCursorPos;
    TacOS::Instance->GetScreenspaceCursorPos( screenspaceCursorPos, screenspaceCursorPosErrors );
    if( screenspaceCursorPosErrors.empty() )
    {
      gTacImGuiGlobals.mMousePositionDesktopWindowspace = {
        screenspaceCursorPos.x - mDesktopWindow->mX,
        screenspaceCursorPos.y - mDesktopWindow->mY };
      gTacImGuiGlobals.mIsWindowDirectlyUnderCursor = mDesktopWindow->mCursorUnobscured;
    }
    else
    {
      gTacImGuiGlobals.mIsWindowDirectlyUnderCursor = false;
    }
    gTacImGuiGlobals.mElapsedSeconds = mShell->mElapsedSeconds;
    gTacImGuiGlobals.mUI2DDrawData = mUi2DDrawData;
    gTacImGuiGlobals.mKeyboardInput = mShell->mKeyboardInput;
  }
  void Update( TacErrors& errors )
  {
    SetImGuiGlobals();
    TacViewport viewport;
    viewport.mViewportPixelWidthIncreasingRight = ( float )mDesktopWindow->mWidth;
    viewport.mViewportPixelHeightIncreasingUp = ( float )mDesktopWindow->mHeight;

    TacScissorRect scissorRect;
    scissorRect.mXMaxRelUpperLeftCornerPixel = ( float )mDesktopWindow->mWidth;
    scissorRect.mYMaxRelUpperLeftCornerPixel = ( float )mDesktopWindow->mHeight;

    TacTexture* framebuffer;
    mDesktopWindow->mRendererData->GetCurrentBackbufferTexture( &framebuffer );

    TacRenderView* renderView = mDesktopWindow->mRenderView;
    renderView->mFramebuffer = framebuffer;
    renderView->mFramebufferDepth = mDesktopWindow->mRendererData->mDepthBuffer;
    renderView->mViewportRect = viewport;
    renderView->mScissorRect = scissorRect;

    mUi2DDrawData->DrawToTexture( errors );
    TAC_HANDLE_ERROR( errors );
  }
  TacDesktopApp* mApp;
  TacShell* mShell;
  TacDesktopWindow* mDesktopWindow;
  TacUI2DDrawData* mUi2DDrawData;
};

void TacDesktopApp::DoStuff( TacDesktopApp* desktopApp, TacErrors& errors )
{
  TacString appDataPath;
  TacOS::Instance->GetApplicationDataPath( appDataPath, errors );

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
  shell->Init( errors );
  TAC_HANDLE_ERROR( errors );

  desktopApp->OnShellInit( errors );
  TAC_HANDLE_ERROR( errors );

  // should this really be on the heap?
  auto game = new TacGame();
  game->mApp = desktopApp;
  game->mShell = shell;
  shell->mOnUpdate.AddCallback( new TacFunctionalHandler( [&]() {
    game->Update( errors ); } ) );

  game->Init( errors );
  TAC_HANDLE_ERROR( errors );

  desktopApp->Loop( errors );
  TAC_HANDLE_ERROR( errors );

  delete game;
}

