#include "tac_game.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-ecs/ghost/tac_ghost.h"
#include "tac-ecs/tac_space.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/os/tac_os.h"

namespace Tac
{
  static WindowHandle       sWindowHandle   {};
  static Ghost*             sGhost          {};
  static bool               sCreateGhost   {};
  const char*               sGameName       { "Game" };
  static UI2DDrawData       sUI2DDrawData   {};
  static UI2DRenderData     sUI2DRenderData {};
  static bool               sIsFullscreen   {};

  struct GameApp : public App
  {
    GameApp( const Config& cfg ) : App( cfg ) {}
    void Init( Errors& errors ) override
    {
      SpaceInit();
      if( sCreateGhost )
      {
        sGhost = TAC_NEW Ghost;
        TAC_CALL( sGhost->Init( errors ) );
      }
    }
    void Update( Errors& errors ) override
    {
      if( sGhost )
      {
        TAC_CALL( sGhost->Update( errors ) );
      }

      static bool doTest{ true };
      if( ImGuiBegin( "Game", &doTest ) )
      {
        ImGuiButton( "button a" );
        ImGuiButton( "button b" );
        ImGuiButton( "button c" );
        ImGuiEnd();
      }
      
    }
    void Render( RenderParams renderParams, Errors& errors ) override
    {
      if( WindowHandle windowHandle{ AppWindowMgr::GetWindowHandle( sGameName ) };
          AppWindowApi::IsShown( windowHandle ) )
      {
        Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
        Render::SwapChainHandle swapChain{ AppWindowApi::GetSwapChainHandle( windowHandle ) };
        Render::TexFmt texFmt{ renderDevice->GetSwapChainColorFmt( swapChain ) };
        Render::TextureHandle tex{ renderDevice->GetSwapChainCurrentColor( swapChain ) };
        const v2i windowSize{ AppWindowApi::GetSize( windowHandle ) };
        UI2DDrawData* pDrawData{ &sUI2DDrawData };
        Span<UI2DDrawData*> drawDatas( pDrawData );
        sUI2DRenderData.DebugDraw2DToTexture( drawDatas, tex, texFmt, windowSize, errors );
      }
    };
  };

  auto App::Create() -> App* { return TAC_NEW GameApp( App::Config{ .mName { sGameName }, } ); }

}// namespace Tac

