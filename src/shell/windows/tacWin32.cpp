#include "src/shell/windows/tacWin32.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacUtility.h"
#include "src/common/tacOS.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacShell.h"
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
  HINSTANCE        Win32GetStartupInstance() { return ghInstance; }
  HINSTANCE        Win32GetStartupPrevInstance() { return ghPrevInstance; }
  LPSTR            Win32GetStartupCmdLine() { return glpCmdLine; }
  int              Win32GetStartupCmdShow() { return gnCmdShow; }


  String Win32ErrorToString( const DWORD winErrorValue )
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
    //WindowsDebugBreak();
    return result;
  }

  String Win32GetLastErrorString()
  {
    const DWORD winErrorValue = GetLastError();
    return Win32ErrorToString( winErrorValue );
  }

  //static String GetWin32WindowClass( HWND hwnd )
  //{
  //  const int byteCountIncNull = 100;
  //  char buffer[ byteCountIncNull ];
  //  GetClassNameA( hwnd, buffer, byteCountIncNull );
  //  String result = buffer;
  //  if( result == "#32768" ) return result + "(a menu)";
  //  if( result == "#32769" ) return result + "(the desktop window)";
  //  if( result == "#32770" ) return result + "(a dialog box)";
  //  if( result == "#32771" ) return result + "(the task switch window)";
  //  if( result == "#32772" ) return result + "(an icon titles)";
  //  return buffer;
  //}

  //static String GetWin32WindowNameAux( HWND hwnd )
  //{
  //  const int byteCountIncNull = 100;
  //  char buffer[ byteCountIncNull ];
  //  GetWindowTextA( hwnd, buffer, byteCountIncNull );
  //  return buffer;
  //}

  //String Win32GetWindowName( HWND hwnd )
  //{
  //  String className = GetWin32WindowClass( hwnd );
  //  String windowName = GetWin32WindowNameAux( hwnd );
  //  return className + " " + windowName;
  //}

  //void Win32Assert( Errors& errors )
  //{
  //  String s = errors.ToString();
  //  std::cout << s.c_str() << std::endl;
  //  Win32DebugBreak();
  //  if( IsDebugMode() )
  //    return;
  //  MessageBox( NULL, s.c_str(), "Tac Assert", MB_OK );
  //  exit( -1 );
  //}

  void Win32DebugBreak()
  {
    if( IsDebugMode() )
      ::DebugBreak();
  }

  //void Win32PopupBox( const StringView& s )
  //{
  //  MessageBox( NULL, s.c_str(), "Message", MB_OK );
  //}

  //void Win32Output( const StringView& s )
  //{
  //  OutputDebugString( s.c_str() );
  //}


}
