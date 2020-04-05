#include "src/game/tacGame.h"
#include "src/shell/tacDesktopApp.h"
#include "src/space/tacGhost.h"
#include "src/common/tacOS.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/graphics/imgui/tacImGui.h"

#include <functional> // std::function

namespace Tac
{
String appName = "Gravestory";

struct Game
{
  static Game* Instance;
  void Init( Errors& errors )
  {
    Instance = this;
    Monitor monitor;
    DesktopApp::Instance->GetPrimaryMonitor( &monitor, errors );
    TAC_HANDLE_ERROR( errors );

    WindowParams windowParams = {};
    windowParams.mName = appName;
    windowParams.mWidth = ( int )( 0.8f * monitor.w );
    windowParams.mHeight = ( int )( 0.8f * monitor.h );
    WindowParams::GetCenteredPosition(
      windowParams.mWidth,
      windowParams.mHeight,
      &windowParams.mX,
      &windowParams.mY,
      monitor );
    DesktopApp::Instance->SpawnWindow( windowParams, &mDesktopWindow, errors );
    TAC_HANDLE_ERROR( errors );

    mUi2DDrawData = new UI2DDrawData;
    mUi2DDrawData->mRenderView = mDesktopWindow->mRenderView;

    auto ghost = new Ghost;
    ghost->mRenderView = mDesktopWindow->mRenderView;
    ghost->Init( errors );
    TAC_HANDLE_ERROR( errors );
  }
  void SetImGuiGlobals()
  {
    Errors screenspaceCursorPosErrors;
    v2 screenspaceCursorPos;
    OS::Instance->GetScreenspaceCursorPos( screenspaceCursorPos, screenspaceCursorPosErrors );
    v2 mousePositionDesktopWindowspace = {};
    bool isWindowDirectlyUnderCursor = false;
    if( screenspaceCursorPosErrors.empty() )
    {
      mousePositionDesktopWindowspace = {
        screenspaceCursorPos.x - mDesktopWindow->mX,
        screenspaceCursorPos.y - mDesktopWindow->mY };
      isWindowDirectlyUnderCursor = mDesktopWindow->mCursorUnobscured;
    }
    ImGuiSetGlobals(
      mousePositionDesktopWindowspace,
      isWindowDirectlyUnderCursor,
      Shell::Instance->mElapsedSeconds,
      mUi2DDrawData );
  }
  void Update( Errors& errors )
  {
    SetImGuiGlobals();
    Viewport viewport;
    viewport.mViewportPixelWidthIncreasingRight = ( float )mDesktopWindow->mWidth;
    viewport.mViewportPixelHeightIncreasingUp = ( float )mDesktopWindow->mHeight;

    ScissorRect scissorRect;
    scissorRect.mXMaxRelUpperLeftCornerPixel = ( float )mDesktopWindow->mWidth;
    scissorRect.mYMaxRelUpperLeftCornerPixel = ( float )mDesktopWindow->mHeight;

    Texture* framebuffer;
    mDesktopWindow->mRendererData->GetCurrentBackbufferTexture( &framebuffer );

    RenderView* renderView = mDesktopWindow->mRenderView;
    renderView->mFramebuffer = framebuffer;
    renderView->mFramebufferDepth = mDesktopWindow->mRendererData->mDepthBuffer;
    renderView->mViewportRect = viewport;
    renderView->mScissorRect = scissorRect;

    mUi2DDrawData->DrawToTexture( errors );
    TAC_HANDLE_ERROR( errors );
  }
  DesktopWindow* mDesktopWindow;
  UI2DDrawData* mUi2DDrawData;
};

Game* Game::Instance = nullptr;

void ExecutableStartupInfo::Init( Errors& errors )
{
  OS* os = OS::Instance;
  String appDataPath;
  OS::Instance->GetApplicationDataPath( appDataPath, errors );

  String studioPath = appDataPath + "\\Sleeping Studio\\";
  String prefPath = studioPath + appName;

  bool appDataPathExists;
  os->DoesFolderExist( appDataPath, appDataPathExists, errors );
  TAC_ASSERT( appDataPathExists );

  os->CreateFolderIfNotExist( studioPath, errors );
  TAC_HANDLE_ERROR( errors );

  os->CreateFolderIfNotExist( prefPath, errors );
  TAC_HANDLE_ERROR( errors );

  String workingDir;
  os->GetWorkingDir( workingDir, errors );
  TAC_HANDLE_ERROR( errors );

  Shell* shell = Shell::Instance;
  shell->mAppName = appName;
  shell->mPrefPath = prefPath;
  shell->mInitialWorkingDir = workingDir;
  shell->Init( errors );
  TAC_HANDLE_ERROR( errors );

  // should this really be on the heap?
  auto game = new Game();
  shell->mOnUpdate.AddCallbackFunctional( []( Errors& errors ) { Game::Instance->Update( errors ); } );

  game->Init( errors );
  TAC_HANDLE_ERROR( errors );

  delete game;
}


}

