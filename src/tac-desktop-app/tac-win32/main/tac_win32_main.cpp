#include "tac-desktop-app/desktop_app/tac_desktop_app.h"              // DesktopApp::Init
#include "tac-dx/dx12/tac_renderer_dx12_ver3.h"                       // DX12Device
#include "tac-std-lib/dataprocess/tac_log.h"                          // LogScope
#include "tac-win32/desktopwindow/tac_win32_desktop_window_manager.h" // Win32WindowManagerInit
#include "tac-win32/input/tac_xinput.h"                               // XInputInit
#include "tac-win32/input/tac_win32_mouse_edge.h"                     // Win32MouseEdgeInit
#include "tac-win32/main/tac_win32_platform.h"                        // Win32PlatformFns
#include "tac-win32/main/tac_win32_redirect_stream_buf.h"             // RedirectStreamBuf
#include "tac-win32/net/tac_net_winsock.h"                            // NetWinsockInit

static Tac::Render::DX12Device sDX12Device;

int CALLBACK WinMain( _In_     HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_     LPSTR lpCmdLine,
                      _In_     int nCmdShow )
{
  using namespace Tac;
  Errors& errors{ DesktopApp::GetMainErrors() };
  TAC_SCOPE_GUARD( LogScope );
  Win32RedirectStdoutVisualStudioOutputWindow();
  Win32SetStartupParams( hInstance, hPrevInstance, lpCmdLine, nCmdShow );
  Win32MouseEdgeInit();
  Win32OSInit();
  Win32InitPlatformFns();
  TAC_CALL_RET( Win32InitWinsock( errors ) );
  TAC_CALL_RET( Win32InitXInput( errors ) );
  TAC_CALL_RET( sDX12Device.Init( errors ) );
  TAC_CALL_RET( Win32WindowManager::Init( errors ) );
  TAC_CALL_RET( DesktopApp::Run( errors ) );
  sDX12Device.Uninit();
  return 0;
}

