#include "src/shell/windows/tac_win32_os.h" // self-inc

#include "src/common/assetmanagers/tac_asset.h"
#include "src/common/containers/tac_array.h"
#include "src/common/containers/tac_fixed_vector.h"
#include "src/common/core/tac_algorithm.h"
#include "src/common/core/tac_error_handling.h"
#include "src/common/core/tac_preprocessor.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/identifier/tac_id_collection.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/string/tac_string_util.h"
#include "src/common/system/tac_filesystem.h"
#include "src/common/system/tac_os.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/windows/tac_win32.h"

import std;
//#include <iostream>
//#include <filesystem>
//#include <ctime> // mktime

#include <Shlobj.h> // SHGetKnownFolderPath
#include <shobjidl_core.h> // IFileSaveDialog, IFileOpenDialog

namespace Tac
{
  static const int    kSemaphoreCapacity = 10;
  static IdCollection gSemaphoreIds( kSemaphoreCapacity );
  static HANDLE       gSemaphores[ kSemaphoreCapacity ];

  static void Win32OSGetPrimaryMonitor( int* w, int* h )
  {
    *w = GetSystemMetrics( SM_CXSCREEN );
    *h = GetSystemMetrics( SM_CYSCREEN );
  }

  static Filesystem::Path Win32OSOpenDialog(  Errors& errors )
  {
    IFileOpenDialog* pDialog = nullptr;

    TAC_HR_CALL( errors, CoInitializeEx, NULL, COINIT_APARTMENTTHREADED );
    TAC_ON_DESTRUCT(CoUninitialize());

    TAC_HR_CALL( errors,
                 CoCreateInstance,
                 CLSID_FileOpenDialog,
                 NULL,
                 CLSCTX_INPROC_SERVER,
                 IID_PPV_ARGS( &pDialog ) );
    TAC_ON_DESTRUCT( pDialog->Release() );


    const Filesystem::Path dir = ShellGetInitialWorkingDir() / AssetPathRootFolderName;
    const std::wstring wDir = dir.Get().wstring();

    IShellItem* shDir = NULL;
    TAC_HR_CALL( errors,
                 SHCreateItemFromParsingName,
                 wDir.c_str(),
                 NULL,
                 IID_PPV_ARGS( &shDir ) );
    TAC_ON_DESTRUCT(shDir->Release());

    TAC_HR_CALL( errors, pDialog->SetDefaultFolder,shDir);

    {
      const HRESULT hr = pDialog->Show( nullptr );
      if( hr == HRESULT_FROM_WIN32( ERROR_CANCELLED ) )
        return {};
      else if( FAILED( hr ) )
      {
        TAC_RAISE_ERROR_RETURN( "failed to show dialog", errors, {} );
      }
    }

    IShellItem* pItem = nullptr;
    TAC_HR_CALL( errors,  pDialog->GetResult, &pItem );
    TAC_ON_DESTRUCT( pItem->Release() );

    PWSTR pszFilePath;
    TAC_HR_CALL( errors, pItem->GetDisplayName, SIGDN_FILESYSPATH, &pszFilePath );
    TAC_ON_DESTRUCT(CoTaskMemFree( pszFilePath ));

    return std::filesystem::path ( pszFilePath );
  }

  static Filesystem::Path Win32OSSaveDialog( const Filesystem::Path& suggestedPath, Errors& errors )
  {
    TAC_HR_CALL( errors, CoInitializeEx, NULL, COINIT_APARTMENTTHREADED );
    TAC_ON_DESTRUCT(CoUninitialize());

    IFileSaveDialog* pDialog = nullptr;
    TAC_HR_CALL( errors,
                 CoCreateInstance,
                 CLSID_FileSaveDialog,
                 NULL,
                 CLSCTX_INPROC_SERVER,
                 IID_PPV_ARGS( &pDialog ) );
    TAC_ON_DESTRUCT( pDialog->Release() );

    // TODO: use suggestedPath, maybe 
    //pDialog->SetFileName();
    TAC_ASSERT_UNIMPLEMENTED;

    TAC_HR_CALL( errors, pDialog->Show, nullptr );

    IShellItem* pItem = nullptr;
    TAC_HR_CALL( errors, pDialog->GetResult, &pItem );
    TAC_ON_DESTRUCT( pItem->Release() );

    PWSTR pszFilePath;
    TAC_HR_CALL( errors, pItem->GetDisplayName, SIGDN_FILESYSPATH, &pszFilePath );
    TAC_ON_DESTRUCT(CoTaskMemFree( pszFilePath ));

    return std::filesystem::path ( pszFilePath );
  };

  static void Win32OSSetScreenspaceCursorPos( const v2& pos, Errors& errors )
  {
    TAC_RAISE_ERROR_IF( !SetCursorPos( ( int )pos.x, ( int )pos.y ), Win32GetLastErrorString(), errors );
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

  static void Win32OSDebugBreak()
  {
    Win32DebugBreak();
  }

  static void Win32OSDebugPopupBox( const StringView& s )
  {
    MessageBox( nullptr, s.data(), nullptr, MB_OK );
  }

  static Filesystem::Path GetRoamingAppDataPathUTF8( Errors& errors )
  {
    PWSTR outPath;
    const HRESULT hr = SHGetKnownFolderPath( FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &outPath );
    TAC_ON_DESTRUCT( CoTaskMemFree( outPath ) );
    TAC_RAISE_ERROR_IF_RETURN( hr != S_OK, "Failed to get roaming folder", errors, "" );

    return std::filesystem::path( outPath );
  }

  static Filesystem::Path Win32OSGetApplicationDataPath( Errors& errors )
  {
    Filesystem::Path path = GetRoamingAppDataPathUTF8( errors );
    TAC_HANDLE_ERROR_RETURN( errors, {} );
    TAC_ASSERT( Filesystem::Exists( path ) );

    path /= ExecutableStartupInfo::sInstance.mStudioName;
    Filesystem::CreateDirectory2( path );
    TAC_ASSERT( Filesystem::Exists( path ) );
    TAC_HANDLE_ERROR_RETURN( errors, {} );

    path /= ExecutableStartupInfo::sInstance.mAppName;
    Filesystem::CreateDirectory2( path );
    TAC_ASSERT( Filesystem::Exists( path ) );
    TAC_HANDLE_ERROR_RETURN( errors, {} );

    return path;
  }

  static SemaphoreHandle Win32OSSemaphoreCreate()
  {
    const int i = gSemaphoreIds.Alloc();
    HANDLE semaphore = CreateSemaphoreA( NULL, 0, 100, NULL );
    TAC_ASSERT( semaphore );
    gSemaphores[ i ] = semaphore;
    return i;
  }

  static void            Win32OSSemaphoreDecrementWait( const SemaphoreHandle handle )
  {
    TAC_ASSERT( handle.IsValid() );
    const HANDLE nativeHandle = gSemaphores[ ( int )handle ];
    WaitForSingleObject( nativeHandle, INFINITE );
  }

  static void            Win32OSSemaphoreIncrementPost( const SemaphoreHandle handle )
  {
    TAC_ASSERT( handle.IsValid() );
    const HANDLE nativeHandle = gSemaphores[ ( int )handle ];
    ReleaseSemaphore( nativeHandle, 1, NULL );
  }


  void             Win32OSInit()
  {
    OS::OSSemaphoreIncrementPost = &Win32OSSemaphoreIncrementPost;
    OS::OSSemaphoreDecrementWait = Win32OSSemaphoreDecrementWait;
    OS::OSSemaphoreCreate = Win32OSSemaphoreCreate;
    OS::OSDebugBreak = Win32OSDebugBreak;
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

