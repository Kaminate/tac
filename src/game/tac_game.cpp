#include "tac_game.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-ecs/ghost/tac_ghost.h"
#include "tac-ecs/tac_space.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/os/tac_os.h"

namespace Tac
{
  static WindowHandle   mWindowHandle;
  static SettingsNode   sSettingsNode;
  static Ghost*         sGhost;

  static void GameCallbackInit(  Errors& errors )
  {
    SpaceInit();
    const Monitor monitor{ OS::OSGetPrimaryMonitor() };
    TAC_ASSERT( !Shell::sShellAppName.empty() );
    const int x{ ( int )( monitor.mSize.x * 0.1f ) };
    const int y{ ( int )( monitor.mSize.y * 0.1f ) };
    const int w{ ( int )( monitor.mSize.x * 0.8f ) };
    const int h{ ( int )( monitor.mSize.y * 0.8f ) };
    TAC_CALL( mWindowHandle = AppWindowApi::CreateWindow(
      WindowCreateParams
      {
        .mName { Shell::sShellAppName },
        .mPos  { x, y },
        .mSize { w, h },
      }, errors ) );
    sGhost = TAC_NEW Ghost;
    TAC_CALL( sGhost->Init( sSettingsNode, errors ));
  }

  static void GameCallbackUpdate( Errors& )
  {
    TAC_NO_OP;
  }

  struct GameApp : public App
  {
    GameApp( const Config& cfg ) : App( cfg ) {}
    void Init(  Errors& errors ) override
    {
      sSettingsNode = mSettingsNode;
      GameCallbackInit(  errors );
    }
    void Update(  Errors& errors ) override
    {
      GameCallbackUpdate( errors );
    }
  };

  App* App::Create() { return TAC_NEW GameApp( App::Config{ .mName { "Game" }, } ); }

}// namespace Tac

