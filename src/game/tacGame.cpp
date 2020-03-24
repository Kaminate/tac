#include "game/tacGame.h"
#include "shell/tacDesktopApp.h"
#include "space/tacGhost.h"
#include "common/tacOS.h"
#include "common/graphics/tacUI2D.h"
#include "common/graphics/tacUI.h"
#include "common/graphics/imgui/tacImGui.h"

#include <functional> // std::function

TacString appName = "Gravestory";

struct TacGame
{
  static TacGame* Instance;
  void Init( TacErrors& errors )
  {
    Instance = this;
    TacMonitor monitor;
    TacDesktopApp::Instance->GetPrimaryMonitor( &monitor, errors );
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
    TacDesktopApp::Instance->SpawnWindow( windowParams, &mDesktopWindow, errors );
    TAC_HANDLE_ERROR( errors );

    mUi2DDrawData = new TacUI2DDrawData;
    mUi2DDrawData->mRenderView = mDesktopWindow->mRenderView;

    auto ghost = new TacGhost;
    ghost->mRenderView = mDesktopWindow->mRenderView;
    ghost->Init( errors );
    TAC_HANDLE_ERROR( errors );
  }
  void SetImGuiGlobals()
  {
    TacErrors screenspaceCursorPosErrors;
    v2 screenspaceCursorPos;
    TacOS::Instance->GetScreenspaceCursorPos( screenspaceCursorPos, screenspaceCursorPosErrors );
    v2 mousePositionDesktopWindowspace = {};
    bool isWindowDirectlyUnderCursor = false;
    if( screenspaceCursorPosErrors.empty() )
    {
      mousePositionDesktopWindowspace = {
        screenspaceCursorPos.x - mDesktopWindow->mX,
        screenspaceCursorPos.y - mDesktopWindow->mY };
      isWindowDirectlyUnderCursor = mDesktopWindow->mCursorUnobscured;
    }
    TacImGuiSetGlobals(
      mousePositionDesktopWindowspace,
      isWindowDirectlyUnderCursor,
      TacShell::Instance->mElapsedSeconds,
      mUi2DDrawData );
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
  TacDesktopWindow* mDesktopWindow;
  TacUI2DDrawData* mUi2DDrawData;
};

TacGame* TacGame::Instance = nullptr;

void TacExecutableStartupInfo::Init( TacErrors& errors )
{
  TacOS* os = TacOS::Instance;
  TacString appDataPath;
  TacOS::Instance->GetApplicationDataPath( appDataPath, errors );

  TacString studioPath = appDataPath + "\\Sleeping Studio\\";
  TacString prefPath = studioPath + appName;

  bool appDataPathExists;
  os->DoesFolderExist( appDataPath, appDataPathExists, errors );
  TacAssert( appDataPathExists );

  os->CreateFolderIfNotExist( studioPath, errors );
  TAC_HANDLE_ERROR( errors );

  os->CreateFolderIfNotExist( prefPath, errors );
  TAC_HANDLE_ERROR( errors );

  TacString workingDir;
  os->GetWorkingDir( workingDir, errors );
  TAC_HANDLE_ERROR( errors );

  TacShell* shell = TacShell::Instance;
  shell->mAppName = appName;
  shell->mPrefPath = prefPath;
  shell->mInitialWorkingDir = workingDir;
  shell->Init( errors );
  TAC_HANDLE_ERROR( errors );

  // should this really be on the heap?
  auto game = new TacGame();
  shell->mOnUpdate.AddCallbackFunctional( []( TacErrors& errors ) { TacGame::Instance->Update( errors ); } );

  game->Init( errors );
  TAC_HANDLE_ERROR( errors );

  delete game;
}

