#include "src/common/graphics/imgui/tacImGui.h"

#include "src/common/tacDesktopWindow.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/tacOS.h"
#include "src/game/tacGame.h"
#include "src/shell/tacDesktopApp.h"
#include "src/space/tacGhost.h"
#include "src/space/tacSpace.h"

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
    OSGetPrimaryMonitor( &monitorWidth, &monitorHeight );

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

  void ExecutableStartupInfo::Init( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mAppName = "Game";
    mProjectInit = GameCallbackInit;
    mProjectUpdate = GameCallbackUpdate;
  }

}

