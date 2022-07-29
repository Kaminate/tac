#include "src/common/math/tac_math.h"
#include "src/common/tac_algorithm.h"
#include "src/common/tac_error_handling.h"
#include "src/common/tac_keyboard_input.h"
#include "src/common/tac_os.h"
#include "src/common/tac_preprocessor.h"
#include "src/common/tac_settings.h"
#include "src/common/string/tac_string.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/windows/tacwinlib/net/tac_net_winsock.h"
#include "src/shell/windows/tacwinlib/tac_win32.h"
#include "src/shell/windows/tacwinlib/desktopwindow/tac_win32_desktop_window_manager.h"
#include "src/shell/windows/tacwinlib/input/tac_xinput.h"
#include "src/shell/windows/tacwinlib/input/tac_win32_mouse_edge.h"
#include "src/shell/windows/tacwinlib/renderer/tac_renderer_directx11.h"
#include "src/common/tac_net.h"

#include <iostream> // okay maybe this should also be allowed


namespace Tac
{
  //static Errors sWinMainErrors;
  //static void ReportError( StringView, Errors& );
  static void WinMainAux( HINSTANCE, HINSTANCE, LPSTR, int, Errors& );
}


int CALLBACK WinMain( HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPSTR lpCmdLine,
                      int nCmdShow )
{
  using namespace Tac;
  WinMainAux( hInstance, hPrevInstance, lpCmdLine, nCmdShow, *GetMainErrors() );
  DesktopAppReportErrors();
  //ReportError( "WinMain", sWinMainErrors );
  //ReportError( "Platform thread", *GetPlatformThreadErrors() );
  //ReportError( "Logic thread", *GetLogicThreadErrors() );
  return 0;
}
#pragma warning( disable: 4996) // itoa deprecated

namespace Tac
{
  //static void ReportError( StringView desc, Errors& errors )
  //{
  //  if( errors )
  //    GetOS()->OSDebugPopupBox( desc + " - " + errors.ToString() );
  //}

  static void Win32FrameBegin( Errors& errors )
  {
    Win32WindowManagerPoll( errors );
  }

  static void Win32FrameEnd( Errors& )
  {
    Win32MouseEdgeUpdate();
  }

  static void RedirectStreamBuf()
  {
    static struct : public std::streambuf
    {
      int overflow( int c ) override
      {
        if( c != EOF )
        {
          char buf[] = { ( char )c, '\0' };
          OutputDebugString( buf );
        }
        return c;
      }
    } streamBuf;
    std::cout.rdbuf( &streamBuf );
    std::cerr.rdbuf( &streamBuf );
    std::clog.rdbuf( &streamBuf );
  }

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

    auto xInput = TAC_NEW XInput();
    xInput->Init( errors );
    TAC_HANDLE_ERROR( errors );

    Win32MouseEdgeInit();

    DesktopAppInit( Win32WindowManagerSpawnWindow,
                    Win32WindowManagerDespawnWindow,
                    Win32WindowManagerGetCursorUnobscuredWindow,
                    Win32FrameBegin,
                    Win32FrameEnd,
                    Win32MouseEdgeSetMovable,
                    Win32MouseEdgeSetResizable,
                    errors );
    TAC_HANDLE_ERROR( errors );

    Win32WindowManagerInit( errors );
    TAC_HANDLE_ERROR( errors );

    Net* net = GetNetWinsock();
    net->Init(errors);
    TAC_HANDLE_ERROR( errors );

    DesktopAppRun( errors );
    TAC_HANDLE_ERROR( errors );
  }
}
