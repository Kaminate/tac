#include "tac_game.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-ecs/ghost/tac_ghost.h"
#include "tac-ecs/tac_space.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"

#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/os/tac_os.h"

namespace Tac
{
  static WindowHandle   sWindowHandle {};
  static Ghost*         sGhost        {};
  const char*           sGameName     { "Game" };

  static void GameCallbackInit( Errors& errors )
  {
    SpaceInit();
    const Monitor monitor{ OS::OSGetPrimaryMonitor() };
    const float percent{ .8f };
    const int x{ ( int )( monitor.mSize.x * ( 1 - percent ) / 2 ) };
    const int y{ ( int )( monitor.mSize.y * ( 1 - percent ) / 2 ) };
    const int w{ ( int )( monitor.mSize.x * percent ) };
    const int h{ ( int )( monitor.mSize.y * percent ) };
    TAC_CALL( sWindowHandle = AppWindowApi::CreateWindow(
      WindowCreateParams
      {
        .mName { sGameName },
        .mPos  { x, y },
        .mSize { w, h },
      }, errors ) );
    sGhost = TAC_NEW Ghost;
    TAC_CALL( sGhost->Init( errors ) );
  }

  static void GameCallbackUpdate( Errors& errors)
  {
    TAC_CALL( sGhost->Update( errors) );
  }

  struct GameApp : public App
  {
    GameApp( const Config& cfg ) : App( cfg ) {}
    void Init( Errors& errors ) override { GameCallbackInit( errors ); }
    void Update( Errors& errors ) override { GameCallbackUpdate( errors ); }
  };

  App* App::Create() { return TAC_NEW GameApp( App::Config{ .mName { sGameName }, } ); }

}// namespace Tac

