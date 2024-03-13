#include "src/shell/windows/tac_win32.h" // self-inc

#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/memory/tac_frame_memory.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/shell/tac_shell.h"
#include "tac-std-lib/containers/tac_fixed_vector.h"
#include "tac-std-lib/identifier/tac_id_collection.h"

#include <Shlobj.h> // SHGetKnownFolderPath
#include <commdlg.h> // GetSaveFileNameA
#include <debugapi.h> // IsDebuggerPresent
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
    // todo: replace with std::breakpoint_if_debugging (C++26)
    if constexpr( IsDebugMode )
    {
      if( ::IsDebuggerPresent() )
      {
        // If the process is not being debugged, the function uses the search logic of a standard
        // exception handler. In most cases, this causes the calling process to terminate because
        // of an unhandled breakpoint exception.
        ::DebugBreak();
      }
    }
  }

  void HrCallAux( const HRESULT hr, const char* fnName, Errors& errors )
  {
    TAC_RAISE_ERROR( String() +
      fnName + " failed with return value " + ToString( ( unsigned long long ) hr ) );
  }

} // namespace Tac
