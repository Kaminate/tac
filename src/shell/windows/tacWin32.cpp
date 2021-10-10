#include "src/shell/windows/tacWin32.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacUtility.h"
#include "src/common/tacOS.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/shell/tacShell.h"
#include "src/common/containers/tacFixedVector.h"
#include "src/common/tacIDCollection.h"

#include <iostream>
#include <ctime> // mktime
#include <Shlobj.h> // SHGetKnownFolderPath
#include <commdlg.h> // GetSaveFileNameA
#pragma comment( lib, "Comdlg32.lib" ) // GetSaveFileNameA

namespace Tac
{
  static HINSTANCE ghInstance;
  static HINSTANCE ghPrevInstance;
  static LPSTR     glpCmdLine;
  static int       gnCmdShow;

  void             Win32SetStartupParams( HINSTANCE hInstance,
                                          HINSTANCE hPrevInstance,
                                          LPSTR lpCmdLine,
                                          int nCmdShow )
  {
    ghInstance = hInstance;
    ghPrevInstance = hPrevInstance;
    glpCmdLine = lpCmdLine;
    gnCmdShow = nCmdShow;
  }
  HINSTANCE        Win32GetStartupInstance()     { return ghInstance; }
  HINSTANCE        Win32GetStartupPrevInstance() { return ghPrevInstance; }
  LPSTR            Win32GetStartupCmdLine()      { return glpCmdLine; }
  int              Win32GetStartupCmdShow()      { return gnCmdShow; }


  String           Win32ErrorToString( const DWORD winErrorValue )
  {
    if( !winErrorValue )
      return "no error";
    LPVOID lpMsgBuf;
    const DWORD flags =
      FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD bufLen = FormatMessage( flags,
                                        NULL,
                                        winErrorValue,
                                        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                                        ( LPTSTR )&lpMsgBuf,
                                        0,
                                        NULL );
    if( !bufLen )
      return "FormatMessage() failed";
    const String result( ( LPCSTR )lpMsgBuf, bufLen );
    LocalFree( lpMsgBuf );
    return result;
  }

  String           Win32GetLastErrorString()
  {
    const DWORD winErrorValue = GetLastError();
    return Win32ErrorToString( winErrorValue );
  }

  void             Win32DebugBreak()
  {
    if( IsDebugMode() )
      ::DebugBreak();
  }

}
