#include "src/shell/windows/tac_win32_main.h" // self-inc

#include "src/common/core/tac_algorithm.h"
#include "src/common/core/tac_error_handling.h"
#include "src/common/core/tac_preprocessor.h"
#include "src/common/dataprocess/tac_settings.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/input/tac_keyboard_input.h"
#include "src/common/math/tac_math.h"
#include "src/common/net/tac_net.h"
#include "src/common/string/tac_string.h"
#include "src/common/system/tac_os.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_event.h"
#include "src/shell/windows/desktopwindow/tac_win32_desktop_window_manager.h"
#include "src/shell/windows/input/tac_win32_mouse_edge.h"
#include "src/shell/windows/input/tac_xinput.h"
#include "src/shell/windows/net/tac_net_winsock.h"
#include "src/shell/windows/renderer/dx11/tac_renderer_dx11.h"
#include "src/shell/windows/tac_win32.h"

import std; // #include <iostream> // okay maybe this should also be allowed


namespace Tac
{
  static void WinMainAux( HINSTANCE, HINSTANCE, LPSTR, int, Errors& );
}


int CALLBACK WinMain( HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPSTR lpCmdLine,
                      int nCmdShow )
{
  using namespace Tac;
  WinMainAux( hInstance, hPrevInstance, lpCmdLine, nCmdShow, GetMainErrors() );
  DesktopAppReportErrors();
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
  static void RedirectStreamBuf()
  {
    struct RedirectBuf : public std::streambuf
    {
      int overflow( int c ) override
      {
        if( c > 0 )
        {
          char buf[] = { ( char )c, '\0' };
          OutputDebugString( buf );
        }

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

  static Win32PlatformFns sWin32PlatformFns;

  // This function exists because TAC_HANDLE_ERROR cannot be used in WinMain
  static void WinMainAux( const HINSTANCE hInstance,
                          const HINSTANCE hPrevInstance,
                          const LPSTR lpCmdLine,
                          const int nCmdShow,
                          Errors& errors )
  {
    Win32OSInit();
    Win32SetStartupParams( hInstance, hPrevInstance, lpCmdLine, nCmdShow );

    RedirectStreamBuf();

    Render::RegisterRendererDirectX11();

    Controller::XInputInit( errors );
    TAC_HANDLE_ERROR( errors );

    Win32MouseEdgeInit();

    DesktopAppInit( &sWin32PlatformFns, errors );
    TAC_HANDLE_ERROR( errors );

    Win32WindowManagerInit( errors );
    TAC_HANDLE_ERROR( errors );

    Network::NetWinsockInit(errors);
    TAC_HANDLE_ERROR( errors );

    DesktopAppRun( errors );
    TAC_HANDLE_ERROR( errors );
  }
} // namespace Tac
