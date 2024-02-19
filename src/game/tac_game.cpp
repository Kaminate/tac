#include "src/game/tac_game.h"

#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/error/tac_error_handling.h"
#include "src/common/graphics/tac_ui_2d.h"
#include "src/common/system/tac_os.h"
#include "src/shell/tac_desktop_app.h"
#include "space/ghost/tac_ghost.h"
#include "space/tac_space.h"

import std; // #include <functional> // std::function

namespace Tac
{
  static DesktopWindowHandle   mDesktopWindowHandle;

  static void GameCallbackInit( Errors& errors )
  {
    SpaceInit();

    auto [monitorWidth, monitorHeight] = OS::OSGetPrimaryMonitor();

    TAC_ASSERT( !sShellAppName.empty() );

    const DesktopAppCreateWindowParams params
    {
      .mName = sShellAppName,
      .mX = 0,
      .mY = 0,
      .mWidth = ( int )( 0.8f * monitorWidth ),
      .mHeight = ( int )( 0.8f * monitorHeight ),
    };

    mDesktopWindowHandle = DesktopApp::GetInstance()->CreateWindow(params);


    auto ghost = TAC_NEW Ghost;
    //ghost->mRenderView = mDesktopWindow->mRenderView;
    TAC_CALL( ghost->Init( errors ));
  }

  static void GameCallbackUpdate( Errors& errors )
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if(!desktopWindowState->mNativeWindowHandle)
          return;

    const v2 size = desktopWindowState->GetSizeV2();
    const Render::Viewport viewport( size );
    const Render::ScissorRect scissorRect( size );


  }

  struct GameApp : public App
  {
    GameApp( const Config& cfg ) : App( cfg ) {}
    void Init( Errors& errors ) override { GameCallbackInit( errors ); }
    void Update( Errors& errors ) override { GameCallbackUpdate( errors ); }
  };
  App* App::Create() { return TAC_NEW GameApp( { .mName = "Game" } ); }


}// namespace Tac

