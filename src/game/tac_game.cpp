#include "src/game/tac_game.h"

#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/core/tac_error_handling.h"
#include "src/common/graphics/tac_ui_2d.h"
#include "src/common/system/tac_os.h"
#include "src/shell/tac_desktop_app.h"
#include "src/space/tac_ghost.h"
#include "src/space/tac_space.h"

import std; // #include <functional> // std::function

namespace Tac
{
  static DesktopWindowHandle   mDesktopWindowHandle;

  void GameCallbackInit( Errors& errors )
  {
    SpaceInit();

    auto [monitorWidth, monitorHeight] = OS::OSGetPrimaryMonitor();

    const DesktopAppCreateWindowParams params
    {
      .mName = App::sInstance.mName,
      .mX = 0,
      .mY = 0,
      .mWidth = ( int )( 0.8f * monitorWidth ),
      .mHeight = ( int )( 0.8f * monitorHeight ),
    };

    mDesktopWindowHandle = DesktopAppCreateWindow(params);


    auto ghost = TAC_NEW Ghost;
    //ghost->mRenderView = mDesktopWindow->mRenderView;
    TAC_CALL( ghost->Init, errors );
  }

  void GameCallbackUpdate( Errors& errors )
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if(!desktopWindowState->mNativeWindowHandle)
          return;

    const v2 size = desktopWindowState->GetSizeV2();
    const Render::Viewport viewport( size );
    const Render::ScissorRect scissorRect( size );


  }

  void App::Init( Errors& errors ) { GameCallbackInit( errors ); }
  void App::Update( Errors& errors ) { GameCallbackUpdate( errors ); }
  void App::Uninit( Errors& ) {}
  App App::sInstance = { .mName = "Game" };


}// namespace Tac

