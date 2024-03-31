#include "tac_win32_os.h" // self-inc

#include "tac-win32/tac_win32_com_ptr.h"
#include "tac-std-lib/filesystem/tac_asset.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/containers/tac_fixed_vector.h"
#include "tac-std-lib/containers/tac_optional.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-rhi/renderer/tac_renderer.h"
//#include "tac-std-lib/identifier/tac_id_collection.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-desktop-app/tac_desktop_app.h"
#include "tac-win32/tac_win32.h"
#include "tac-win32/filedialog/tac_win32_file_dialog.h"

import std; // iostream, filesystem, ctime (mktime )

#include <Shlobj.h> // SHGetKnownFolderPath
#include <shobjidl_core.h> // IFileSaveDialog, IFileOpenDialog

namespace Tac
{
  struct Win32Semaphore : public OS::ISemaphore
  {
    Win32Semaphore()
    {
      mNativeHandle = CreateSemaphoreA( NULL, 0, 100, NULL );
    }

    void DecrementWait() override
    {
      WaitForSingleObject( mNativeHandle, INFINITE );
    }

    void IncrementPost() override
    {
      ReleaseSemaphore( mNativeHandle, 1, NULL );
    }

    HANDLE mNativeHandle;
  };

  static OS::Monitor Win32OSGetPrimaryMonitor()
  {
    return 
    {
      .mWidth = GetSystemMetrics( SM_CXSCREEN ),
      .mHeight = GetSystemMetrics( SM_CYSCREEN ),
    };
  }

  static Filesystem::Path Win32OSOpenDialog( Errors& errors )
  {
    FileDialogHelper helper( FileDialogHelper::kOpen );
    return helper.Run( errors );
  }

  static Filesystem::Path Win32OSSaveDialog( const OS::SaveParams&, Errors& errors )
  {
    FileDialogHelper helper( FileDialogHelper::kSave );
    return helper.Run( errors );
  };

  static void Win32OSSetScreenspaceCursorPos( const v2& pos, Errors& errors )
  {
    TAC_RAISE_ERROR_IF( !SetCursorPos( ( int )pos.x, ( int )pos.y ), Win32GetLastErrorString() );
  }

  static void* Win32OSGetLoadedDLL( const StringView& name )
  {
    HMODULE moduleHandle = GetModuleHandleA( name.c_str() );
    return moduleHandle;
  }

  static void* Win32OSLoadDLL( const StringView& path )
  {
    HMODULE lib = LoadLibraryA( path.c_str() );
    return lib;
  }

  static bool IsDirectorySeparator( char c )
  {
    return c == '/' || c == '\\';
  }

  // returns true if the path starts from a Drive letter, ie C:/...
  static bool IsFullPath( StringView path )
  {
    if( path.size() < 3 )
      return false;

    const char drive = path[ 0 ];
    if( !IsAlpha( drive ) )
      return false;

    const char colon = path[ 1 ];
    if( colon != ':' )
      return false;

    const char slash = path[ 2 ];
    if( !IsDirectorySeparator( slash ) )
      return false;

    return true;
  }


  static void Win32OSDebugPopupBox( const StringView& s )
  {
    if constexpr( IsDebugMode )
    {
      MessageBox( nullptr, s.data(), nullptr, MB_OK );
    }
  }

  static Filesystem::Path GetRoamingAppDataPathUTF8( Errors& errors )
  {
    PWSTR outPath = nullptr;
    const HRESULT hr = SHGetKnownFolderPath( FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &outPath );
    TAC_ON_DESTRUCT( CoTaskMemFree( outPath ) );
    TAC_RAISE_ERROR_IF_RETURN( hr != S_OK, "Failed to get roaming folder", "" );
    return std::filesystem::path( outPath );
  }

  static Filesystem::Path Win32OSGetApplicationDataPath( Errors& errors )
  {
    Filesystem::Path path = TAC_CALL_RET( {}, GetRoamingAppDataPathUTF8( errors ));
    TAC_ASSERT( Filesystem::Exists( path ) );

    path /= sShellStudioName;
    Filesystem::CreateDirectory2( path );
    TAC_ASSERT( Filesystem::Exists( path ) );

    
    path /= sShellAppName;
    Filesystem::CreateDirectory2( path );
    TAC_ASSERT( Filesystem::Exists( path ) );

    return path;
  }

  static OS::ISemaphore* Win32OSSemaphoreCreate() { return TAC_NEW Win32Semaphore; }

  void             Win32OSInit()
  {
    OS::OSSemaphoreCreate = Win32OSSemaphoreCreate;
    OS::OSDebugBreak = Win32DebugBreak;
    OS::OSDebugPopupBox = Win32OSDebugPopupBox;
    OS::OSGetApplicationDataPath = Win32OSGetApplicationDataPath;
    OS::OSSaveDialog = Win32OSSaveDialog;
    OS::OSOpenDialog = Win32OSOpenDialog;
    OS::OSGetPrimaryMonitor = Win32OSGetPrimaryMonitor;
    OS::OSSetScreenspaceCursorPos = Win32OSSetScreenspaceCursorPos;
    OS::OSGetLoadedDLL = Win32OSGetLoadedDLL;
    OS::OSLoadDLL = Win32OSLoadDLL;
  }
}

