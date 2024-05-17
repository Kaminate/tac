#include "tac_game.h" // self-inc

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-ecs/ghost/tac_ghost.h"
#include "tac-ecs/tac_space.h"

import std; // #include <functional> // std::function

namespace Tac
{
  static WindowHandle   mWindowHandle;
  static SettingsNode   sSettingsNode;

  static void GameCallbackInit( App::InitParams initParams, Errors& errors )
  {
    SpaceInit();

    const Monitor monitor{ OS::OSGetPrimaryMonitor() };

    TAC_ASSERT( !sShellAppName.empty() );

    const SysWindowApi* windowApi{ initParams.mWindowApi };

    const v2i size( ( int )( monitor.mSize.x * 0.8f ),
                    ( int )( monitor.mSize.y * 0.8f ) );

    const WindowCreateParams windowCreateParams
    {
      .mName { sShellAppName },
      .mPos  {},
      .mSize { size },
    };

    TAC_CALL( mWindowHandle = windowApi->CreateWindow( windowCreateParams, errors ) );


    auto ghost { TAC_NEW Ghost };
    //ghost->mRenderView = mDesktopWindow->mRenderView;
    TAC_CALL( ghost->Init( sSettingsNode,  errors ));
  }

  static void GameCallbackUpdate( App::UpdateParams updateParams, Errors& errors )
  {

  }

  struct GameApp : public App
  {
    GameApp( const Config& cfg ) : App( cfg ) {}
    void Init( InitParams initParams, Errors& errors ) override
    {
      sSettingsNode = mSettingsNode;
      GameCallbackInit( initParams, errors );
    }
    void Update( UpdateParams updateParams, Errors& errors ) override
    {
      GameCallbackUpdate( updateParams, errors );
    }
  };

  App* App::Create()
  {
    const App::Config config
    {
       .mName { "Game"  },
    };
    return TAC_NEW GameApp( config );
  }


}// namespace Tac

