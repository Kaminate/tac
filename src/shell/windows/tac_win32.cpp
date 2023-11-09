#include "src/shell/windows/tac_win32.h"
#include "src/common/core/tac_preprocessor.h"
#include "src/common/string/tac_string_util.h"
#include "src/common/system/tac_os.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/core/tac_algorithm.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/containers/tac_fixed_vector.h"
#include "src/common/identifier/tac_id_collection.h"

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


  String           Win32ErrorStringFromDWORD( const DWORD winErrorValue )
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

  //String           Win32ErrorStringFromHRESULT( const HRESULT hr )
  //{
  //  const DWORD dw = ( DWORD )hr; // Is this kosher? NO IDEA
  //  return Win32ErrorStringFromDWORD( dw );
  //}

  String           Win32GetLastErrorString()
  {
    const DWORD winErrorValue = GetLastError();
    return Win32ErrorStringFromDWORD( winErrorValue );
  }

  void             Win32DebugBreak()
  {
    if( IsDebugMode() )
      ::DebugBreak();
  }

}
