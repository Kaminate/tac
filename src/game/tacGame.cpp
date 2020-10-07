#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/tacOS.h"
#include "src/game/tacGame.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/tacDesktopWindowManager.h"
#include "src/space/tacGhost.h"
#include "src/space/tacSpace.h"

#include <functional> // std::function

namespace Tac
{
  String appName = "Gravestory";
  static Game gGame;
  void GameCallbackInit( Errors& errors ) { gGame.Init( errors ); }
  void GameCallbackUpdate( Errors& errors ) { gGame.Update( errors ); }

  Game* Game::Instance = nullptr;

  void Game::Init( Errors& errors )
  {
    SpaceInit();
    Instance = this;

    int monitorWidth;
    int monitorHeight;
    OS::GetPrimaryMonitor( &monitorWidth, &monitorHeight );

    int windowWidth = ( int )( 0.8f * monitorWidth );
    int windowHeight = ( int )( 0.8f * monitorHeight );
    int windowX;
    int windowY;
    CenterWindow( &windowX, &windowY, windowWidth, windowHeight );
    mDesktopWindowHandle = DesktopWindowCreate( windowX,
                                                                         windowY,
                                                                         windowWidth,
                                                                         windowHeight );
    TAC_HANDLE_ERROR( errors );

    mUi2DDrawData = TAC_NEW UI2DDrawData;

    auto ghost = TAC_NEW Ghost;
    //ghost->mRenderView = mDesktopWindow->mRenderView;
    ghost->Init( errors );
    TAC_HANDLE_ERROR( errors );
  }
  void Game::SetImGuiGlobals()
  {
    DesktopWindowState* desktopWindowState = DesktopWindowStateCollection::InstanceStuffThread.FindDesktopWindowState( mDesktopWindowHandle );
    Errors screenspaceCursorPosErrors;
    v2 screenspaceCursorPos;
    OS::GetScreenspaceCursorPos( screenspaceCursorPos, screenspaceCursorPosErrors );
    v2 mousePositionDesktopWindowspace = {};
    bool isWindowDirectlyUnderCursor = false;
    if( screenspaceCursorPosErrors.empty() )
    {
      mousePositionDesktopWindowspace = {
        screenspaceCursorPos.x - desktopWindowState->mX,
        screenspaceCursorPos.y - desktopWindowState->mY };
      isWindowDirectlyUnderCursor = desktopWindowState->mCursorUnobscured;
    }
    ImGuiSetGlobals( mousePositionDesktopWindowspace,
                     isWindowDirectlyUnderCursor,
                     Shell::Instance.mElapsedSeconds,
                     mUi2DDrawData,
                     desktopWindowState->mWidth,
                     desktopWindowState->mHeight );

  }
  void Game::Update( Errors& errors )
  {
    DesktopWindowState* desktopWindowState = DesktopWindowStateCollection::InstanceStuffThread.FindDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState )
      return;
    SetImGuiGlobals();
    Viewport viewport;
    viewport.mWidth = ( float )desktopWindowState->mWidth;
    viewport.mHeight = ( float )desktopWindowState->mHeight;

    ScissorRect scissorRect;
    scissorRect.mXMaxRelUpperLeftCornerPixel = ( float )desktopWindowState->mWidth;
    scissorRect.mYMaxRelUpperLeftCornerPixel = ( float )desktopWindowState->mHeight;

    const Render::ViewId viewId = 0;

    mUi2DDrawData->DrawToTexture( desktopWindowState->mWidth,
                                  desktopWindowState->mHeight,
                                  viewId,
                                  errors );
    TAC_HANDLE_ERROR( errors );
  }


  void ExecutableStartupInfo::Init( Errors& errors )
  {
    mAppName = "Game";
    mProjectInit = GameCallbackInit;
    mProjectUpdate = GameCallbackUpdate;
  }

}

