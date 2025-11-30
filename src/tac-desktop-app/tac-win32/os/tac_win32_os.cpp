#include "tac_win32_os.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/containers/tac_fixed_vector.h"
#include "tac-std-lib/containers/tac_optional.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-win32/filedialog/tac_win32_file_dialog.h"
#include "tac-win32/tac_win32.h"
#include "tac-win32/tac_win32_com_ptr.h"

#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <filesystem>
  #include <iostream>
#endif
//import std; // iostream, filesystem, ctime (mktime )

#include <Shlobj.h> // SHGetKnownFolderPath
#include <shobjidl_core.h> // IFileSaveDialog, IFileOpenDialog

#include <shellapi.h> // ShellExecuteA

namespace Tac
{


  typedef enum { MDT_EFFECTIVE_DPI = 0, MDT_ANGULAR_DPI = 1, MDT_RAW_DPI = 2, MDT_DEFAULT = MDT_EFFECTIVE_DPI } MONITOR_DPI_TYPE;
  using PFN_GetDpiForMonitor = HRESULT( WINAPI* )( HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT* );        // Shcore.lib + dll, Windows 8.1+

  static auto Win32GetMonitorDpi( HMONITOR monitor ) -> int
  {
    TAC_ASSERT( monitor );
    if( _IsWindows8Point1OrGreater() )
    {
      static HINSTANCE shcore_dll{ ::LoadLibraryA( "shcore.dll" ) }; // Reference counted per-process
      static PFN_GetDpiForMonitor GetDpiForMonitorFn;
      if( !GetDpiForMonitorFn && shcore_dll )
        GetDpiForMonitorFn = ( PFN_GetDpiForMonitor )::GetProcAddress( shcore_dll, "GetDpiForMonitor" );
      if( GetDpiForMonitorFn )
      {
        UINT xdpi{};
        UINT ydpi{};
        HRESULT hr{ GetDpiForMonitorFn( ( HMONITOR )monitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi ) };
        TAC_ASSERT( hr == S_OK );
        return (int)xdpi; // assumed same as y
      }
    }

    TAC_ASSERT_UNIMPLEMENTED; // Should this be demoted to a warning?
    return USER_DEFAULT_SCREEN_DPI;
  }

  static auto Win32MonitorToTacMonitor( HMONITOR win32Monitor ) -> Monitor
  {
    const int dpi{ Win32GetMonitorDpi( win32Monitor ) };

    MONITORINFO info  {};
    info.cbSize = sizeof(info);
    if( !TAC_VERIFY( ::GetMonitorInfo( win32Monitor, &info ) ) )
      return {};

    int w{ info.rcMonitor.right - info.rcMonitor.left };
    int h{ info.rcMonitor.bottom - info.rcMonitor.top };
    int x{ info.rcMonitor.left };
    int y{ info.rcMonitor.top };

    return Monitor
    {
      .mPos { x, y },
      .mSize { w, h},
      .mDpi  { dpi},
    };
  }

  static auto Win32OSGetPrimaryMonitor() -> Monitor
  {
    HMONITOR win32Monitor{ ::MonitorFromPoint( POINT{}, MONITOR_DEFAULTTOPRIMARY ) };
    return Win32MonitorToTacMonitor( win32Monitor );
    
#if 0
      return Monitor
      {
        .mSize { GetSystemMetrics( SM_CXSCREEN ), GetSystemMetrics( SM_CYSCREEN ) },
      };
#endif
  }


  static auto Win32OSGetMonitorAtPoint( v2 p ) -> Monitor
  {
    POINT point{ ( LONG )p.x, ( LONG )p.y };
    HMONITOR win32Monitor{ ::MonitorFromPoint( point, MONITOR_DEFAULTTONEAREST ) };
    return Win32MonitorToTacMonitor( win32Monitor );
  }

  static auto Win32OSGetMonitorFromNativeWindowHandle ( const void* p ) -> Monitor
  {
    HMONITOR win32Monitor{ ::MonitorFromWindow( ( HWND )p, MONITOR_DEFAULTTOPRIMARY ) };
    return Win32MonitorToTacMonitor( win32Monitor );
  }

  static void Win32OSSetScreenspaceCursorPos( const v2& pos, Errors& errors )
  {
    TAC_RAISE_ERROR_IF( !SetCursorPos( ( int )pos.x, ( int )pos.y ), Win32GetLastErrorString() );
  }

  static auto Win32OSGetLoadedDLL( const StringView& name ) -> void*
  {
    return GetModuleHandleA( name.c_str() ) ;
  }

  static auto Win32OSLoadDLL( const StringView& path ) -> void*
  {
    return LoadLibraryA( path.c_str() );
  }

  static auto Win32OSGetProcAddr( void* dll, const StringView& path ) -> void*
  {
    return GetProcAddress( ( HMODULE )dll, path.c_str() );
  }

  static void Win32OSOpenPath( const UTF8Path& path, Errors& errors )
  {
    String pathStr{ path };
    pathStr.replace( "/", "\\" ); // <-- important
    const INT_PTR shellExecuteResult{ ( INT_PTR )
      ::ShellExecuteA( NULL, "open", pathStr.data(), NULL, NULL, SW_SHOWDEFAULT ) };
    TAC_RAISE_ERROR_IF( shellExecuteResult <= 32, Win32GetLastErrorString() );
  }

  static void Win32OSDebugPopupBox( const StringView& s )
  {
    if constexpr( kIsDebugMode )
    {
      MessageBox( nullptr, s.data(), nullptr, MB_OK );
    }
  }

  static auto GetRoamingAppDataUTF8Path( Errors& errors ) -> UTF8Path
  {
    PWSTR outPath {};
    const HRESULT hr {
      SHGetKnownFolderPath( FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &outPath ) };
    TAC_ON_DESTRUCT( CoTaskMemFree( outPath ) );
    TAC_RAISE_ERROR_IF_RETURN( hr != S_OK, "Failed to get roaming folder" );
    return (char*)std::filesystem::path( outPath ).u8string().c_str();
  }

  static auto Win32OSGetApplicationDataPath( Errors& errors ) -> UTF8Path
  {
    TAC_CALL_RET( UTF8Path path{ GetRoamingAppDataUTF8Path( errors ) } );
    TAC_ASSERT( path.Exists() );

    path /= Shell::sShellStudioName;
    path.CreateDir();
    TAC_ASSERT( path.Exists() );

    path /= Shell::sShellAppName;
    path.CreateDir();
    TAC_ASSERT( path.Exists() );

    return path;
  }

} // namespace Tac

void Tac::Win32OSInit()
{
  OS::OSDebugBreak = Win32DebugBreak;
  OS::OSDebugPopupBox = Win32OSDebugPopupBox;
  OS::OSGetApplicationDataPath = Win32OSGetApplicationDataPath;
  OS::OSSaveDialog = Win32FileDialogSave;
  OS::OSOpenDialog = Win32FileDialogOpen;
  OS::OSGetPrimaryMonitor = Win32OSGetPrimaryMonitor;
  OS::OSGetMonitorAtPoint = Win32OSGetMonitorAtPoint;
  OS::OSGetMonitorFromNativeWindowHandle = Win32OSGetMonitorFromNativeWindowHandle;
  OS::OSSetScreenspaceCursorPos = Win32OSSetScreenspaceCursorPos;
  OS::OSGetLoadedDLL = Win32OSGetLoadedDLL;
  OS::OSLoadDLL = Win32OSLoadDLL;
  OS::OSGetProcAddress = Win32OSGetProcAddr;
  OS::OSOpenPath = Win32OSOpenPath;
}

