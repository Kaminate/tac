#include "tac_jppt.h" // self-inc

#include "tac-rhi/render3/tac_render_api.h"
//#include "tac-engine-core/window/tac_window_handle.h"
//#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-std-lib/os/tac_os.h"
//#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
//#include "tac-ecs/ghost/tac_ghost.h"
//#include "tac-ecs/tac_space.h"

namespace Tac
{
  static WindowHandle   mWindowHandle;
  static SettingsNode   sSettingsNode;
  static v2i   sWindowPos;
  static v2i   sWindowSize;


  JPPTApp::JPPTApp( Config cfg ) : App( cfg )
  {
  }

  void    JPPTApp::Init( InitParams initParams, Errors& errors)
  {
    const Monitor monitor{ OS::OSGetPrimaryMonitor() };
    const SysWindowApi windowApi{ initParams.mWindowApi };
    const v2i size( ( int )( monitor.mSize.x * 0.8f ),
                    ( int )( monitor.mSize.y * 0.8f ) );
    const v2i pos{ ( monitor.mSize - size ) / 2 };
    const WindowCreateParams windowCreateParams
    {
      .mName { "JPPT" },
      .mPos  { pos },
      .mSize { size },
    };

    sWindowPos = pos;
    sWindowSize = size;
    //TAC_CALL( mWindowHandle = windowApi.CreateWindow( windowCreateParams, errors ) );
    sSettingsNode = mSettingsNode;
  }

  void    JPPTApp::Update( UpdateParams updateParams, Errors& )
  {
    ImGuiSetNextWindowDisableBG();
    ImGuiSetNextWindowPosition( sWindowPos );
    ImGuiSetNextWindowSize( sWindowSize );
    if( ImGuiBegin("jppt") )
    {
      ImGuiText("hi");
      ImGuiEnd();
    }
  }

  void    JPPTApp::Render( RenderParams, Errors& )
  {
  }

  void    JPPTApp::Present( PresentParams, Errors& )
  {
  }

  void    JPPTApp::Uninit( Errors& )
  {
  }

  App* App::Create()
  {
    const App::Config config
    {
       .mName { "JPPT" },
    };
    return TAC_NEW JPPTApp( config );
  }


}// namespace Tac

