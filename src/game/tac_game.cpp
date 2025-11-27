#include "tac_game.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-ecs/ghost/tac_ghost.h"
#include "tac-ecs/tac_space.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/os/tac_os.h"

namespace Tac
{
  static WindowHandle       sWindowHandle{};
  static Ghost*             sGhost{};
  const char*               sGameName{ "Game" };
  static UI2DDrawData       sUI2DDrawData{};
  static UI2DRenderData     sUI2DRenderData{};

  struct GameApp : public App
  {
    GameApp( const Config& cfg ) : App( cfg ) {}
    void Init( Errors& errors ) override
    {
      SpaceInit();
#if 0
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
#endif
    }
    void Update( Errors& errors ) override
    {
#if 0
      TAC_CALL( sGhost->Update( errors ) );
#endif

      Monitor monitor{ OS::OSGetMonitorAtPoint( v2( -2000, 50 ) ) };
      //Monitor monitor{ OS::OSGetMonitorAtPoint( v2( 0, 0 ) ) };
      int w{ Min( 1920 * monitor.mDpi / 96, ( int )( monitor.mSize.x * .95f ) ) };
      int h{ Min( 1080 * monitor.mDpi / 96, ( int )( monitor.mSize.y * .95f ) ) };
      int x{ monitor.mPos.x + ( monitor.mSize.x - w ) / 2 };
      int y{ monitor.mPos.y + ( monitor.mSize.y - h ) / 2 };
      AppWindowMgr::SetNextWindowPosition( v2( ( float )x, ( float )y ) );
      AppWindowMgr::SetNextWindowSize( v2( ( float )w, ( float )h ) );
      if( AppWindowMgr::WindowBegin( sGameName ) )
      {
        TAC_ON_DESTRUCT(AppWindowMgr::WindowEnd());
        float uiScale = 1.f;
        float width_inches = 2;
        float width_px = monitor.mDpi * width_inches * uiScale;
        float height_inches = 1;
        float height_px = monitor.mDpi * height_inches * uiScale;
        sUI2DDrawData.AddBox(
          UI2DDrawData::Box
          {
            .mMini          { 50, 50 },
            .mMaxi          { 50.f + width_px, 50.f + height_px },
            .mColor         { 1, 1, 1, 1 },
          } );
      }
    }
    void Render( RenderParams renderParams, Errors& errors ) override
    {
      if( WindowHandle windowHandle{ AppWindowMgr::GetWindowHandle( sGameName ) };
          AppWindowApi::IsShown( windowHandle ) )
      {
        Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
        Render::SwapChainHandle swapChain{ AppWindowApi::GetSwapChainHandle( windowHandle ) };
        Render::SwapChainParams swapChainParams{ renderDevice->GetSwapChainParams( swapChain ) };
        Render::TextureHandle tex{ renderDevice->GetSwapChainCurrentColor( swapChain ) };
        v2i windowSize{ AppWindowApi::GetSize( windowHandle ) };
        UI2DDrawData* pDrawData{ &sUI2DDrawData };
        Span<UI2DDrawData*> drawDatas( pDrawData );
        sUI2DRenderData.DebugDraw2DToTexture( drawDatas, tex, swapChainParams.mColorFmt, windowSize, errors );
      }
    };
  };

  auto App::Create() -> App* { return TAC_NEW GameApp( App::Config{ .mName { sGameName }, } ); }

}// namespace Tac

