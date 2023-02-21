#include "src/common/graphics/imgui/tac_imgui.h"

#include "src/common/tac_desktop_window.h"
#include "src/common/tac_error_handling.h"
#include "src/common/graphics/tac_ui_2d.h"
#include "src/common/tac_os.h"
#include "src/game/tac_game.h"
#include "src/shell/tac_desktop_app.h"
#include "src/space/tac_ghost.h"
#include "src/space/tac_space.h"

#include <functional> // std::function

namespace Tac
{
  static DesktopWindowHandle   mDesktopWindowHandle;

  void GameCallbackInit( Errors& errors )
  {
    SpaceInit();
    //Instance = this;

    int monitorWidth;
    int monitorHeight;
    OS::OSGetPrimaryMonitor( &monitorWidth, &monitorHeight );

    int windowWidth = ( int )( 0.8f * monitorWidth );
    int windowHeight = ( int )( 0.8f * monitorHeight );
    int windowX = 0;
    int windowY = 0;
    mDesktopWindowHandle = DesktopAppCreateWindow( windowX,
                                                windowY,
                                                windowWidth,
                                                windowHeight );
    TAC_HANDLE_ERROR( errors );


    auto ghost = TAC_NEW Ghost;
    //ghost->mRenderView = mDesktopWindow->mRenderView;
    ghost->Init( errors );
    TAC_HANDLE_ERROR( errors );
  }

  void GameCallbackUpdate( Errors& errors )
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if(!desktopWindowState->mNativeWindowHandle)
          return;
    Render::Viewport viewport;
    viewport.mWidth = ( float )desktopWindowState->mWidth;
    viewport.mHeight = ( float )desktopWindowState->mHeight;

    Render::ScissorRect scissorRect;
    scissorRect.mXMaxRelUpperLeftCornerPixel = ( float )desktopWindowState->mWidth;
    scissorRect.mYMaxRelUpperLeftCornerPixel = ( float )desktopWindowState->mHeight;

    TAC_HANDLE_ERROR( errors );
  }

  ExecutableStartupInfo ExecutableStartupInfo::Init()
  {
    return { .mAppName = "Game",
    .mProjectInit = GameCallbackInit,
    .mProjectUpdate = GameCallbackUpdate, };
  }

}

