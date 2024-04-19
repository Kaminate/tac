#include "tac_win32_main.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-desktop-app/tac_desktop_event.h"

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
//#include "tac-engine-core/hid/tac_keyboard_api.h"
#include "tac-engine-core/net/tac_net.h"
#include "tac-engine-core/settings/tac_settings.h"

//#include "tac-rhi/render/tac_render_backend.h"

#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/dataprocess/tac_log.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string.h"

#include "tac-win32/desktopwindow/tac_win32_desktop_window_manager.h"
#include "tac-win32/input/tac_win32_mouse_edge.h"
#include "tac-win32/input/tac_xinput.h"
#include "tac-win32/net/tac_net_winsock.h"
//#include "tac-win32/dx/dx11/tac_renderer_dx11.h"
//#include "tac-win32/dx/dx12/tac_renderer_dx12_ver2.h"
//#include "tac-win32/dx/dx12/tac_renderer_dx12_ver2.h"
#include "tac-win32/dx/dx12/tac_renderer_dx12_ver3.h" // DX12Backend
#include "tac-win32/dx/dxgi/tac_dxgi_debug.h" // DXGIReportLiveObjects
#include "tac-win32/dx/pix/tac_pix.h" // AllowPIXDebuggerAttachment
#include "tac-win32/tac_win32.h"

import std; // #include <iostream> // okay maybe this should also be allowed

static Tac::Win32PlatformFns sWin32PlatformFns;
static Tac::Render::DX12Backend sDX12Backend;

int CALLBACK WinMain( HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPSTR lpCmdLine,
                      int nCmdShow )
{
  using namespace Tac;
  Errors& errors = GetMainErrors();

  TAC_SCOPE_GUARD( LogScope );

  TAC_MEDIEVAL_DEBUG;

  Win32OSInit();
  Win32SetStartupParams( hInstance, hPrevInstance, lpCmdLine, nCmdShow );

  RedirectStreamBuf();

  TAC_CALL_RET( 0, Render::AllowPIXDebuggerAttachment( errors ));
  //Render::RegisterRendererDirectX11();

  
  //Render::IRenderBackend3::Set( &dx12Backend );

  TAC_CALL_RET( 0, Controller::XInputInit( errors ));

  Win32MouseEdgeInit();

  DesktopApp* desktopApp = DesktopApp::GetInstance();

  PlatformFns::SetInstance( &sWin32PlatformFns );

  TAC_CALL_RET( 0, desktopApp->Init( errors ) );

  TAC_CALL_RET( 0, Win32WindowManagerInit( errors ) );

  TAC_CALL_RET( 0, Network::NetWinsockInit( errors ) );

  TAC_CALL_RET( 0, desktopApp->Run( errors ) );

  Render::DXGIReportLiveObjects();

  return 0;
}

namespace Tac
{

  static void Win32FrameBegin( Errors& errors )
  {
    Win32WindowManagerPoll( errors );
  }

  static void Win32FrameEnd( Errors& )
  {
    Win32MouseEdgeUpdate();

    const DesktopEventApi::CursorUnobscuredEvent data{ Win32MouseEdgeGetCursorHovered() };
    DesktopEventApi::Queue( data );
  }

  // Redirect stdout to output window
  void RedirectStreamBuf()
  {
    struct RedirectBuf : public std::streambuf
    {
      // xsputn() outputs strings of size n
      std::streamsize xsputn( const char* s, std::streamsize n ) override
      {
        OutputDebugStringA( s );
        return n;
      }

      // overflow() outputs characters such as '\n'
      int overflow( int c ) override
      {
        const char s[] = { (char)c, '\0' };
        OutputDebugStringA( s );
        return c;
      }
    };

    static RedirectBuf streamBuf;
    std::cout.rdbuf( &streamBuf );
    std::cerr.rdbuf( &streamBuf );
    std::clog.rdbuf( &streamBuf );
  }

  // -----------------------------------------------------------------------------------------------
  void Win32PlatformFns::PlatformImGui( Errors& errors ) 
  {
    if( !ImGuiCollapsingHeader( "Win32PlatformFns::PlatformImGui" ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;

    Win32WindowManagerDebugImGui();
  }

  void Win32PlatformFns::PlatformFrameBegin( Errors& errors ) 
  {
    Win32FrameBegin( errors );
  }

  void Win32PlatformFns::PlatformFrameEnd( Errors& errors ) 
  {
    Win32FrameEnd( errors );
  }

  void Win32PlatformFns::PlatformSpawnWindow( const PlatformSpawnWindowParams& params,
                                              Errors& errors ) 
  {
    Win32WindowManagerSpawnWindow( params, errors );
  }

  void Win32PlatformFns::PlatformDespawnWindow( WindowHandle handle ) 
  {
    Win32WindowManagerDespawnWindow( handle );
  }

  //void Win32PlatformFns::PlatformWindowMoveControls( const WindowHandle& handle,
  //                                                   const DesktopWindowRect& rect )
  //{
  //  Win32MouseEdgeSetMovable( handle, rect );
  //}

  //void Win32PlatformFns::PlatformWindowResizeControls( const WindowHandle& handle,
  //                                                     int i )
  //{
  //  Win32MouseEdgeSetResizable( handle, i );
  //}

  //WindowHandle Win32PlatformFns::PlatformGetMouseHoveredWindow() 
  //{
  //  return Win32WindowManagerGetCursorUnobscuredWindow();
  //}

  // -----------------------------------------------------------------------------------------------


} // namespace Tac
