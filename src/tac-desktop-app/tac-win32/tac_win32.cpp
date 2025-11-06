#include "tac_win32.h" // self-inc

#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/containers/tac_fixed_vector.h"
#include "tac-std-lib/dataprocess/tac_text_parser.h" // ParseData

#include <debugapi.h> // IsDebuggerPresent

namespace Tac
{
  static HINSTANCE ghInstance;
  static HINSTANCE ghPrevInstance;
  static LPSTR     glpCmdLine;
  static int       gnCmdShow;
}

void Tac::Win32SetStartupParams( HINSTANCE hInstance,
                                 HINSTANCE hPrevInstance,
                                 LPSTR lpCmdLine,
                                 int nCmdShow )
{
  ghInstance = hInstance;
  ghPrevInstance = hPrevInstance;
  glpCmdLine = lpCmdLine;
  gnCmdShow = nCmdShow;

  if( ParseData parseData( ( const char* )lpCmdLine ); parseData )
  {
    for( StringView word{ parseData.EatWord() }; !word.empty(); word = parseData.EatWord() )
    {
      if( word.starts_with( "--" ) )
        OS::CmdLineAddFlag( word.substr( 2 ) );
      else if( word.starts_with( "-") )
        OS::CmdLineAddFlag( word.substr( 1 ) );
    }
  }
}

auto Tac::Win32GetStartupInstance() ->HINSTANCE      { return ghInstance; }
auto Tac::Win32GetStartupPrevInstance() -> HINSTANCE { return ghPrevInstance; }
auto Tac::Win32GetStartupCmdLine() -> LPSTR          { return glpCmdLine; }
auto Tac::Win32GetStartupCmdShow() -> int            { return gnCmdShow; }

auto Tac::Win32ErrorStringFromDWORD( const DWORD winErrorValue ) -> String
{
  if( !winErrorValue )
    return "no error";
  dynmc LPVOID lpMsgBuf{};
  const DWORD flags{
    FORMAT_MESSAGE_ALLOCATE_BUFFER |
    FORMAT_MESSAGE_FROM_SYSTEM |
    FORMAT_MESSAGE_IGNORE_INSERTS };
  const DWORD bufLen{ FormatMessage( flags,
                                      NULL,
                                      winErrorValue,
                                      MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                                      ( LPTSTR )&lpMsgBuf,
                                      0,
                                      NULL ) };
  if( !bufLen )
    return "FormatMessage() failed";

  const String result( ( LPCSTR )lpMsgBuf, bufLen );
  LocalFree( lpMsgBuf );
  return result;
}

auto Tac::Win32GetLastErrorString() -> String { return Win32ErrorStringFromDWORD( GetLastError() ); }

void Tac::Win32DebugBreak()
{
  // todo: replace with std::breakpoint_if_debugging (C++26)
  if constexpr( kIsDebugMode )
    if( ::IsDebuggerPresent() )
      // If the process is not being debugged, the function uses the search logic of a standard
      // exception handler. In most cases, this causes the calling process to terminate because
      // of an unhandled breakpoint exception.
      ::DebugBreak();
}

void Tac::HrCallAux( const HRESULT hr, const char* fnName, Errors& errors )
{
  TAC_RAISE_ERROR( String() +
                   fnName + " failed with return value " + ToString( ( unsigned long long ) hr ) );
}

