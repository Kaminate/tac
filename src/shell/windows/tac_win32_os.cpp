#include "src/shell/windows/tac_win32_os.h" // self-inc

#include "src/shell/windows/tac_win32_com_ptr.h"
#include "src/common/assetmanagers/tac_asset.h"
#include "src/common/containers/tac_array.h"
#include "src/common/containers/tac_fixed_vector.h"
#include "src/common/containers/tac_optional.h"
#include "src/common/algorithm/tac_algorithm.h"
#include "src/common/error/tac_error_handling.h"
#include "src/common/preprocess/tac_preprocessor.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/identifier/tac_id_collection.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/string/tac_string_util.h"
#include "src/common/system/tac_filesystem.h"
#include "src/common/system/tac_os.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/windows/tac_win32.h"

import std; // iostream, filesystem, ctime (mktime )

#include <Shlobj.h> // SHGetKnownFolderPath
#include <shobjidl_core.h> // IFileSaveDialog, IFileOpenDialog

namespace Tac
{
  static const int    kSemaphoreCapacity = 10;
  static IdCollection gSemaphoreIds( kSemaphoreCapacity );
  static HANDLE       gSemaphores[ kSemaphoreCapacity ];

  static OS::Monitor Win32OSGetPrimaryMonitor()
  {
    return 
    {
      .mWidth = GetSystemMetrics( SM_CXSCREEN ),
      .mHeight = GetSystemMetrics( SM_CYSCREEN ),
    };
  }

  struct FileDialogHelper
  {
    enum Type
    {
      kOpen,
      kSave,
    };

    FileDialogHelper( Type type ) : mType( type ) { }

    ~FileDialogHelper()
    {
      CoUninitialize();
    }

    Filesystem::Path Run( Errors& errors )
    {
      TAC_HR_CALL_RET( {}, CoInitializeEx( NULL, COINIT_APARTMENTTHREADED ) );
      TAC_CALL_RET( {}, CreateDialogInstance( errors ) );
      TAC_CALL_RET( {}, SetDefaultFolder( errors ) );
      TAC_CALL_RET( {}, Show( errors ) );
      if( mCancelled )
        return {};
      return GetResult( errors );
    }

  private:
    void SetDefaultFolder(Errors& errors)
    {
      const Filesystem::Path dir = sShellInitialWorkingDir / AssetPathRootFolderName;
      const std::wstring wDir = dir.Get().wstring();

      PCom<IShellItem> shDir;
      TAC_HR_CALL( SHCreateItemFromParsingName(
                   wDir.c_str(),
                   NULL,
                   shDir.iid(),
                   shDir.ppv() ) );
      TAC_HR_CALL( mDialog->SetDefaultFolder( ( IShellItem* )shDir ) );
    }

    void CreateDialogInstance(Errors& errors)
    {
      REFCLSID sid = mType == Type::kOpen ? CLSID_FileOpenDialog : CLSID_FileSaveDialog;
      REFIID iid = mType == Type::kOpen ? mOpenDialog.iid() : mSaveDialog.iid();
      void** ppv = mType == Type::kOpen ? mOpenDialog.ppv() : mSaveDialog.ppv();
      TAC_HR_CALL( CoCreateInstance( sid, NULL, CLSCTX_INPROC_SERVER, iid, ppv ) );
      mDialog = mType == Type::kOpen ? (IFileDialog*)mOpenDialog : (IFileDialog*)mSaveDialog;
    };

    void Show(Errors& errors)
    {
      const HRESULT hr = mDialog->Show( nullptr );
      mCancelled = hr == HRESULT_FROM_WIN32( ERROR_CANCELLED );
      TAC_RAISE_ERROR_IF( FAILED( hr ) && !mCancelled, "Failed to show dialog");
    }

    Filesystem::Path GetResult( Errors& errors )
    {
      PCom<IShellItem> pItem;
      TAC_HR_CALL_RET( {}, mDialog->GetResult( pItem.CreateAddress() ) );

      PWSTR pszFilePath;
      TAC_HR_CALL_RET( {}, pItem->GetDisplayName( SIGDN_FILESYSPATH, &pszFilePath ) );
      TAC_ON_DESTRUCT( CoTaskMemFree( pszFilePath ) );

      return std::filesystem::path( pszFilePath );
    }

    IFileDialog* mDialog = nullptr;
    PCom<IFileOpenDialog> mOpenDialog;
    PCom<IFileSaveDialog> mSaveDialog;
    bool mCancelled = false;
    Type mType;
  };

  static Filesystem::Path Win32OSOpenDialog( Errors& errors )
  {
    FileDialogHelper helper( FileDialogHelper::kOpen );
    return helper.Run( errors );
  }

  static Filesystem::Path Win32OSSaveDialog( const OS::SaveParams& , Errors& errors )
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

