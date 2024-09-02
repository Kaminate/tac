#include "tac_win32_main.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-desktop-app/desktop_event/tac_desktop_event.h"

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/net/tac_net.h"

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
#include "tac-dx/dx12/tac_renderer_dx12_ver3.h" // DX12Backend
#include "tac-dx/dxgi/tac_dxgi_debug.h" // DXGIReportLiveObjects
#include "tac-dx/pix/tac_pix_dbg_attach.h" // AllowPIXDebuggerAttachment
#include "tac-win32/tac_win32.h"

import std; // #include <iostream>

static Tac::Win32PlatformFns   sWin32PlatformFns;
static Tac::Render::DX12Device sDX12Device;

int CALLBACK WinMain( _In_     HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_     LPSTR lpCmdLine,
                      _In_     int nCmdShow )
{
  using namespace Tac;

  Errors& errors{ DesktopApp::GetMainErrors() };
  TAC_SCOPE_GUARD( LogScope );
  Win32OSInit();
  Win32SetStartupParams( hInstance, hPrevInstance, lpCmdLine, nCmdShow );
  RedirectStreamBuf();

  TAC_CALL_RET( 0, Render::AllowPIXDebuggerAttachment( errors ) );
  Render::RenderApi::SetRenderDevice( &sDX12Device );
  TAC_CALL_RET( 0, sDX12Device.Init( errors ) );
  TAC_CALL_RET( 0, Controller::XInputInit( errors ) );
  Win32MouseEdgeInit();
  PlatformFns::SetInstance( &sWin32PlatformFns );
  TAC_CALL_RET( 0, DesktopApp::Init( errors ) );
  TAC_CALL_RET( 0, Win32WindowManagerInit( errors ) );
  TAC_CALL_RET( 0, Network::NetWinsockInit( errors ) );
  TAC_CALL_RET( 0, DesktopApp::Run( errors ) );
  Render::DXGIReportLiveObjects();
  return 0;
}


// Redirect stdout to output window
void Tac::RedirectStreamBuf()
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
      const char s[] = { ( char )c, '\0' };
      OutputDebugStringA( s );
      return c;
    }
  };

  static RedirectBuf streamBuf;
  std::cout.rdbuf( &streamBuf );
  std::cerr.rdbuf( &streamBuf );
  std::clog.rdbuf( &streamBuf );
}

namespace Tac
{

  // -----------------------------------------------------------------------------------------------

  void Win32PlatformFns::PlatformImGui( Errors& errors ) const
  {
    if( !ImGuiCollapsingHeader( "Win32PlatformFns::PlatformImGui" ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;

    Win32WindowManagerDebugImGui();
  }

  void Win32PlatformFns::PlatformFrameBegin( Errors& errors ) const
  {
    Win32WindowManagerPoll( errors );
  }

  void Win32PlatformFns::PlatformFrameEnd( Errors& errors ) const
  {
    Win32MouseEdgeUpdate();

    const DesktopEventApi::CursorUnobscuredEvent data{ Win32MouseEdgeGetCursorHovered() };
    DesktopEventApi::Queue( data );
  }

  void Win32PlatformFns::PlatformSpawnWindow( const PlatformSpawnWindowParams& params,
                                              Errors& errors ) const
  {
    Win32WindowManagerSpawnWindow( params, errors );
  }

  void Win32PlatformFns::PlatformDespawnWindow( WindowHandle handle ) const
  {
    Win32WindowManagerDespawnWindow( handle );
  }

  void Win32PlatformFns::PlatformSetWindowPos( WindowHandle windowHandle, v2i pos) const
  {
    const HWND hwnd{ Win32WindowManagerGetHWND( windowHandle ) };


    RECT rect;
    if( !::GetWindowRect( hwnd, &rect ) )
    {
      //String errStr{ Win32GetLastErrorString() };
      return;
    }

    const int x{ pos.x };
    const int y{ pos.y };
    const int w{ rect.right - rect.left };
    const int h{ rect.bottom - rect.top };

    ::SetWindowPos( hwnd, nullptr, x, y, w, h, SWP_ASYNCWINDOWPOS );
  }

  void Win32PlatformFns::PlatformSetMouseCursor( PlatformMouseCursor cursor ) const
  {
    static HCURSOR cursorArrow { LoadCursor( NULL, IDC_ARROW ) };
    static HCURSOR cursorArrowNS { LoadCursor( NULL, IDC_SIZENS ) };
    static HCURSOR cursorArrowEW { LoadCursor( NULL, IDC_SIZEWE ) };
    static HCURSOR cursorArrowNE_SW { LoadCursor( NULL, IDC_SIZENESW ) };
    static HCURSOR cursorArrowNW_SE { LoadCursor( NULL, IDC_SIZENWSE ) };

    switch( cursor )
    {
    case PlatformMouseCursor::kNone:        ::SetCursor( nullptr );          break;
    case PlatformMouseCursor::kArrow:       ::SetCursor( cursorArrow );      break;
    case PlatformMouseCursor::kResizeNS:    ::SetCursor( cursorArrowNS );    break;
    case PlatformMouseCursor::kResizeEW:    ::SetCursor( cursorArrowEW );    break;
    case PlatformMouseCursor::kResizeNE_SW: ::SetCursor( cursorArrowNE_SW ); break;
    case PlatformMouseCursor::kResizeNW_SE: ::SetCursor( cursorArrowNW_SE ); break;
    default: TAC_ASSERT_INVALID_CASE( cursor );                              break;
    }
  }

  void Win32PlatformFns::PlatformSetWindowSize( WindowHandle windowHandle, v2i size) const
  {
    const HWND hwnd{ Win32WindowManagerGetHWND( windowHandle ) };


    RECT rect;
    if( !::GetWindowRect( hwnd, &rect ) )
    {
      //String errStr{ Win32GetLastErrorString() };
      return;
    }

    const int x{ rect.left };
    const int y{ rect.top };
    const int w{ size.x };
    const int h{ size.y };

    ::SetWindowPos( hwnd, nullptr, x, y, w, h, SWP_ASYNCWINDOWPOS );
  }

  WindowHandle Win32PlatformFns::PlatformGetMouseHoveredWindow() const
  {
    POINT cursorPos;
    if( !::GetCursorPos( &cursorPos ) )
      return {};

    const HWND hwnd{ ::WindowFromPoint( cursorPos ) };
    return Win32WindowManagerFindWindow( hwnd );
  }

  // -----------------------------------------------------------------------------------------------


} // namespace Tac
