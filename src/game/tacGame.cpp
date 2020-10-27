#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/tacOS.h"
#include "src/game/tacGame.h"
#include "src/shell/tacDesktopApp.h"
#include "src/space/tacGhost.h"
#include "src/space/tacSpace.h"

#include <functional> // std::function

namespace Tac
{
  static UI2DDrawData*         mUi2DDrawData;
  static DesktopWindowHandle   mDesktopWindowHandle;

  void GameCallbackInit( Errors& errors )
  {
    SpaceInit();
    //Instance = this;

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

  void GameCallbackUpdate( Errors& errors )
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
if(!desktopWindowState->mNativeWindowHandle)
      return;
    Viewport viewport;
    viewport.mWidth = ( float )desktopWindowState->mWidth;
    viewport.mHeight = ( float )desktopWindowState->mHeight;

    ScissorRect scissorRect;
    scissorRect.mXMaxRelUpperLeftCornerPixel = ( float )desktopWindowState->mWidth;
    scissorRect.mYMaxRelUpperLeftCornerPixel = ( float )desktopWindowState->mHeight;

    //mUi2DDrawData->DrawToTexture( mDesktopWindowHandle, errors );
    TAC_HANDLE_ERROR( errors );
  }

  void ExecutableStartupInfo::Init( Errors& errors )
  {
    mAppName = "Game";
    mProjectInit = GameCallbackInit;
    mProjectUpdate = GameCallbackUpdate;
  }

}

