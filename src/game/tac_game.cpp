#include "tac_game.h" // self-inc

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
  static bool               sCreateGhost    {};
  const char*               sGameName       { "Game" };
  static UI2DDrawData       sUI2DDrawData   {};
  static UI2DRenderData     sUI2DRenderData {};
  static bool               sIsFullscreen   {};

  GameApp::GameApp( const Config& cfg ) : App( cfg ) {}
  void GameApp::Init( Errors& errors ) 
  {
    SpaceInit();
    if( sCreateGhost )
    {
      sGhost = TAC_NEW Ghost;
      TAC_CALL( sGhost->Init( errors ) );
    }
  }
  void GameApp::Update( Errors& errors )
  {
    if( sGhost )
    {
      TAC_CALL( sGhost->Update( errors ) );
    }

    static bool showWindow{ true };
    static bool shownWindow{};

    if( ImGuiBegin( "Game", &showWindow ) )
    {
      TAC_ON_DESTRUCT(ImGuiEnd());

      shownWindow = true;
      if( ImGuiBeginMenuBar() )
      {
        TAC_ON_DESTRUCT(ImGuiEndMenuBar());

        if( ImGuiBeginMenu( "File" ) )
        {
          TAC_ON_DESTRUCT(ImGuiEndMenu());
          ImGuiButton( "new" );
          ImGuiButton( "open" );
          ImGuiButton( "close" );
          ImGuiButton( "exit" );
          ImGuiButton( "save" );
        }
        if( ImGuiBeginMenu( "Edit" ) )
        {
          TAC_ON_DESTRUCT(ImGuiEndMenu());
          ImGuiButton( "undo" );
          ImGuiButton( "redo" );
          ImGuiButton( "cut" );
          ImGuiButton( "copy" );
          ImGuiButton( "paste" );
        }
      }

      ImGuiBeginGroup();
      TAC_ON_DESTRUCT(ImGuiEndGroup());

      ImGuiButton( "button a" );
      ImGuiText( "the quick brown fox jumped over the lazy dog. :) !@#@#O$*&SD l;kfj we" );
      ImGuiButton( "button b" );
      ImGuiButton( "button c" );
    }

    if( !showWindow && shownWindow )
      OS::OSAppStopRunning();
    
  }
  void GameApp::Render( RenderParams renderParams, Errors& errors ) 
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
    TAC_CALL( Render::IContext::Scope renderContextScope{ renderDevice->CreateRenderContext( errors ) } );
    Render::IContext* renderContext{ renderContextScope.GetContext() };
        sUI2DRenderData.DebugDraw2DToTexture( renderContext, drawDatas, tex, texFmt, windowSize, errors );
        renderContext->Execute( errors );
      }
    };

  auto App::Create() -> App* { return TAC_NEW GameApp( App::Config{ .mName { sGameName }, } ); }

}// namespace Tac

