#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/tacOS.h"
#include "src/game/tacGame.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/tacDesktopWindowManager.h"
#include "src/space/tacGhost.h"

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

      DesktopWindowHandle desktopWindowHandle = DesktopWindowManager::Instance->CreateWindow( windowParams.mX,
                                                                                              windowParams.mY,
                                                                                              windowParams.mWidth,
                                                                                              windowParams.mHeight );
      TAC_HANDLE_ERROR( errors );

      mUi2DDrawData = TAC_NEW UI2DDrawData;

      auto ghost = TAC_NEW Ghost;
      //ghost->mRenderView = mDesktopWindow->mRenderView;
      ghost->Init( errors );
      TAC_HANDLE_ERROR( errors );
    }
    void SetImGuiGlobals()
    {
      Errors screenspaceCursorPosErrors;
      v2 screenspaceCursorPos;
      OS::GetScreenspaceCursorPos( screenspaceCursorPos, screenspaceCursorPosErrors );
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
        mUi2DDrawData,
        mDesktopWindowState.mWidth,
        mDesktopWindowState.mHeight );
        
    }
    void Update( Errors& errors )
    {
      SetImGuiGlobals();
      Viewport viewport;
      viewport.mWidth = ( float )mDesktopWindow->mWidth;
      viewport.mHeight = ( float )mDesktopWindow->mHeight;

      ScissorRect scissorRect;
      scissorRect.mXMaxRelUpperLeftCornerPixel = ( float )mDesktopWindow->mWidth;
      scissorRect.mYMaxRelUpperLeftCornerPixel = ( float )mDesktopWindow->mHeight;

      Texture* framebuffer;
      mDesktopWindow->mRendererData->GetCurrentBackbufferTexture( &framebuffer );

      //RenderView* renderView = mDesktopWindow->mRenderView;
      //renderView->mFramebuffer = framebuffer;
      //renderView->mFramebufferDepth = mDesktopWindow->mRendererData->mDepthBuffer;
      //renderView->mViewportRect = viewport;
      //renderView->mScissorRect = scissorRect;

      mUi2DDrawData->DrawToTexture( 0, 0, 0, errors );
      TAC_HANDLE_ERROR( errors );
    }
    DesktopWindowState mDesktopWindowState;
    DesktopWindow* mDesktopWindow;
    UI2DDrawData* mUi2DDrawData;
  };

  Game* Game::Instance = nullptr;

  void ExecutableStartupInfo::Init( Errors& errors )
  {
    String appDataPath;
    OS::GetApplicationDataPath( appDataPath, errors );

    String studioPath = appDataPath + "\\Sleeping Studio\\";
    String prefPath = studioPath + appName;

    bool appDataPathExists;
    OS::DoesFolderExist( appDataPath, appDataPathExists, errors );
    TAC_ASSERT( appDataPathExists );

    OS::CreateFolderIfNotExist( studioPath, errors );
    TAC_HANDLE_ERROR( errors );

    OS::CreateFolderIfNotExist( prefPath, errors );
    TAC_HANDLE_ERROR( errors );

    String workingDir;
    OS::GetWorkingDir( workingDir, errors );
    TAC_HANDLE_ERROR( errors );

    Shell::Instance->mAppName = appName;
    Shell::Instance->mPrefPath = prefPath;
    Shell::Instance->mInitialWorkingDir = workingDir;
    Shell::Instance->Init( errors );
    TAC_HANDLE_ERROR( errors );

    // should this really be on the heap?
    auto game = TAC_NEW Game();
    Shell::Instance->mOnUpdate.AddCallbackFunctional( []( Errors& errors ) { Game::Instance->Update( errors ); } );

    game->Init( errors );
    TAC_HANDLE_ERROR( errors );

    delete game;
  }


}

