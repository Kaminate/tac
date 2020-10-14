#include "src/common/math/tacMath.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacKeyboardinput.h"
#include "src/common/tacOS.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacSettings.h"
#include "src/common/tacString.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/windows/tacNetWinsock.h"
#include "src/shell/windows/tacWin32.h"
#include "src/shell/windows/tacWin32DesktopWindowManager.h"
#include "src/shell/windows/tacXInput.h"

namespace Tac
{
  static Errors sWinMainErrors;
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
  auto ReportError = []( StringView desc, Errors& errors ) { if( errors ) {
      OS::DebugPopupBox( desc + errors.ToString() ); } };
  ReportError( "WinMain", sWinMainErrors );
  ReportError( "Platform thread", gPlatformThreadErrors );
  ReportError( "Logic thread", gLogicThreadErrors );
  return 0;
}

namespace Tac
{

  // This function exists because TAC_HANDLE_ERROR cannot be used in WinMain
  static void WinMainAux( const HINSTANCE hInstance,
                          const HINSTANCE hPrevInstance,
                          const LPSTR lpCmdLine,
                          const int nCmdShow )
  {
    ghInstance = hInstance;
    ghPrevInstance = hPrevInstance;
    glpCmdLine = lpCmdLine;
    gnCmdShow = nCmdShow;
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

    AppInterfacePlatform appInterfacePlatform = {};
    appInterfacePlatform.mPlatformPoll = WindowsManagerPoll;
    appInterfacePlatform.mPlatformSpawnWindow = WindowsManagerSpawnWindow;
    DesktopAppInit( appInterfacePlatform, errors );
    TAC_HANDLE_ERROR( errors );

    WindowsManagerInit( errors );
    TAC_HANDLE_ERROR( errors );

    NetWinsock::Instance.Init( errors );
    TAC_HANDLE_ERROR( errors );

    DesktopAppRun( errors );
    TAC_HANDLE_ERROR( errors );
  }
}
