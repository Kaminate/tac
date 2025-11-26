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
  static auto Win32OSGetPrimaryMonitor() -> Monitor
  {
    return { .mSize { GetSystemMetrics( SM_CXSCREEN ), GetSystemMetrics( SM_CYSCREEN ) } };
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
  OS::OSSetScreenspaceCursorPos = Win32OSSetScreenspaceCursorPos;
  OS::OSGetLoadedDLL = Win32OSGetLoadedDLL;
  OS::OSLoadDLL = Win32OSLoadDLL;
  OS::OSGetProcAddress = Win32OSGetProcAddr;
  OS::OSOpenPath = Win32OSOpenPath;
}

