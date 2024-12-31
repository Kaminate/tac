#include "tac_win32_main.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"              // DesktopApp::Init
#include "tac-std-lib/dataprocess/tac_log.h"                          // LogScope
#include "tac-win32/desktopwindow/tac_win32_desktop_window_manager.h" // Win32WindowManagerInit
#include "tac-win32/input/tac_win32_mouse_edge.h"                     // Win32MouseEdgeInit
#include "tac-win32/input/tac_xinput.h"                               // XInputInit
#include "tac-win32/net/tac_net_winsock.h"                            // NetWinsockInit
#include "tac-dx/dx12/tac_renderer_dx12_ver3.h"                       // DX12Device
#include "tac-dx/dxgi/tac_dxgi_debug.h"                               // DXGIReportLiveObjects
#include "tac-dx/pix/tac_pix_dbg_attach.h"                            // AllowPIXDebuggerAttachment
#include "tac-win32/main/tac_win32_redirect_stream_buf.h"             // RedirectStreamBuf
#include "tac-win32/main/tac_win32_platform.h"                        // Win32PlatformFns

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
  TAC_CALL_RET( Render::AllowPIXDebuggerAttachment( errors ) );
  Render::RenderApi::SetRenderDevice( &sDX12Device );
  TAC_CALL_RET( sDX12Device.Init( errors ) );
  TAC_CALL_RET( XInputInit( errors ) );
  Win32MouseEdgeInit();
  PlatformFns::SetInstance( &sWin32PlatformFns );
  TAC_CALL_RET( DesktopApp::Init( errors ) );
  TAC_CALL_RET( Win32WindowManagerInit( errors ) );
  TAC_CALL_RET( Network::NetWinsockInit( errors ) );
  TAC_CALL_RET( DesktopApp::Run( errors ) );
  Render::DXGIReportLiveObjects();

  return 0;
}

