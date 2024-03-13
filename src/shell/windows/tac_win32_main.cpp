#include "src/shell/windows/tac_win32_main.h" // self-inc

#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/dataprocess/tac_settings.h"
#include "tac-std-lib/dataprocess/tac_log.h"
#include "tac-rhi/ui/imgui/tac_imgui.h"
#include "tac-rhi/render/tac_render_backend.h"
#include "tac-std-lib/input/tac_keyboard_input.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/net/tac_net.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/os/tac_os.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_event.h"
#include "src/shell/windows/desktopwindow/tac_win32_desktop_window_manager.h"
#include "src/shell/windows/input/tac_win32_mouse_edge.h"
#include "src/shell/windows/input/tac_xinput.h"
#include "src/shell/windows/net/tac_net_winsock.h"
#include "src/shell/windows/renderer/dx11/tac_renderer_dx11.h"
#include "src/shell/windows/renderer/dx12/tac_renderer_dx12_ver2.h"
#include "src/shell/windows/renderer/pix/tac_pix.h"
#include "src/shell/windows/tac_win32.h"
#include "src/shell/windows/renderer/dxgi/tac_dxgi_debug.h"

import std; // #include <iostream> // okay maybe this should also be allowed

static Tac::Win32PlatformFns sWin32PlatformFns;

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
  Render::RegisterRendererDirectX11();

  Render::DX12Backend dx12Backend;
  
  Render::IBackend::Set( &dx12Backend );

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
    DesktopEvent( DesktopEventDataCursorUnobscured{ Win32MouseEdgeGetCursorHovered() } );
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

  void Win32PlatformFns::PlatformDespawnWindow( const DesktopWindowHandle& errors ) 
  {
    Win32WindowManagerDespawnWindow( errors );
  }

  void Win32PlatformFns::PlatformWindowMoveControls( const DesktopWindowHandle& handle,
                                                     const DesktopWindowRect& rect )
  {
    Win32MouseEdgeSetMovable( handle, rect );
  }

  void Win32PlatformFns::PlatformWindowResizeControls( const DesktopWindowHandle& handle,
                                                       int i )
  {
    Win32MouseEdgeSetResizable( handle, i );
  }

  DesktopWindowHandle Win32PlatformFns::PlatformGetMouseHoveredWindow() 
  {
    return Win32WindowManagerGetCursorUnobscuredWindow();
  }

  // -----------------------------------------------------------------------------------------------


} // namespace Tac
