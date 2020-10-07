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
#include "src/shell/windows/tacWin32Main.h"
#include "src/shell/windows/tacXInput.h"

//#include <thread>
//#include <iostream>
//#include <set>

namespace Tac
{
  static void RedirectDebugStreams()
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
  static void ReportError( StringView desc, Errors& errors )
  {
    if( errors )
      OS::DebugPopupBox( desc + errors.ToString() );
  }

  //auto ReportError = []( StringView desc, Errors& errors )
  //{
  //  if( errors )
  //    OS::DebugPopupBox( desc + " errors: " + errors.ToString() );
  //};

  static void WinMainAux( Errors& errors )
  {
    RedirectDebugStreams();

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


int CALLBACK WinMain( HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPSTR lpCmdLine,
                      int nCmdShow )
{
  using namespace Tac;
  ghInstance = hInstance;
  ghPrevInstance = hPrevInstance;
  glpCmdLine = lpCmdLine;
  gnCmdShow = nCmdShow;
  Errors errors;
  WinMainAux( errors );
  ReportError( "WinMain", errors );
  ReportError( "Platform thread", sPlatformThreadErrors );
  ReportError( "Logic thread", sLogicThreadErrors );
  return 0;
}
