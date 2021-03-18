#include "src/common/math/tacMath.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacKeyboardinput.h"
#include "src/common/tacOS.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacSettings.h"
#include "src/common/string/tacString.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/windows/tacNetWinsock.h"
#include "src/shell/windows/tacWin32.h"
#include "src/shell/windows/tacWin32DesktopWindowManager.h"
#include "src/shell/windows/tacXInput.h"
#include "src/shell/windows/tacWin32MouseEdge.h"

#include <iostream>

namespace Tac
{
  static Errors sWinMainErrors;
  static void ReportError( StringView, Errors& );
  static void WinMainAux( HINSTANCE hInstance,
                          HINSTANCE hPrevInstance,
                          LPSTR lpCmdLine,
                          int nCmdShow );
}


int CALLBACK WinMain( HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPSTR lpCmdLine,
                      int nCmdShow )
{
  using namespace Tac;
  WinMainAux( hInstance, hPrevInstance, lpCmdLine, nCmdShow );
  ReportError( "WinMain", sWinMainErrors );
  ReportError( "Platform thread", *GetPlatformThreadErrors() );
  ReportError( "Logic thread", *GetLogicThreadErrors() );
  return 0;
}

namespace Tac
{
  static void ReportError( StringView desc, Errors& errors )
  {
    if( errors )
      OSDebugPopupBox( desc + " - " + errors.ToString() );
  }

  static void Win32FrameBegin( Errors& errors )
  {
    Win32WindowManagerPoll( errors );
  }

  static void Win32FrameEnd( Errors& )
  {
    Win32MouseEdgeUpdate();
  }

  // This function exists because TAC_HANDLE_ERROR cannot be used in WinMain
  static void WinMainAux( const HINSTANCE hInstance,
                          const HINSTANCE hPrevInstance,
                          const LPSTR lpCmdLine,
                          const int nCmdShow )
  {
    Win32SetStartupParams( hInstance, hPrevInstance, lpCmdLine, nCmdShow );
    Errors& errors = sWinMainErrors;
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

    NetWinsock::Instance.Init( errors );
    TAC_HANDLE_ERROR( errors );

    DesktopAppRun( errors );
    TAC_HANDLE_ERROR( errors );
  }
}
