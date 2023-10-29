#include "src/common/containers/tac_array.h"
#include "src/common/containers/tac_fixed_vector.h"
#include "src/common/system/tac_filesystem.h"
#include "src/common/string/tac_string_util.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/core/tac_algorithm.h"
#include "src/common/identifier/tac_id_collection.h"
#include "src/common/system/tac_os.h"
#include "src/common/core/tac_preprocessor.h"
#include "src/common/string/tac_string_util.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/windows/tac_win32.h"

#include <iostream>
#include <filesystem>
#include <ctime> // mktime
#include <Shlobj.h> // SHGetKnownFolderPath
#include <commdlg.h> // GetSaveFileNameA
#pragma comment( lib, "Comdlg32.lib" ) // GetSaveFileNameA

namespace Tac
{
  static const int    kSemaphoreCapacity = 10;
  static IdCollection gSemaphoreIds( kSemaphoreCapacity );
  static HANDLE       gSemaphores[ kSemaphoreCapacity ];

  struct Win32DirectoryCallbackFunctor
  {
    virtual void operator()( const Filesystem::Path& dir,
                             const WIN32_FIND_DATA&,
                             Errors& ) = 0;
  };

#if 0
  static String WideStringToUTF8( WCHAR* inputWideStr, Errors& errors )
  {
    String result;
    auto wCharCount = ( int )wcslen( inputWideStr );
    result.resize( wCharCount * 8 ); // ought to be enough
    const DWORD flags = 0;
    int conversionResult = WideCharToMultiByte( CP_UTF8,
                                                flags,
                                                inputWideStr,
                                                -1,
                                                result.data(),
                                                result.size(),
                                                NULL,
                                                NULL );
    TAC_RAISE_ERROR_IF_RETURN( !conversionResult, Win32GetLastErrorString(), errors, "" );
    result.resize( conversionResult );
    return result;
  }
#endif

  static String GetFileDialogErrors( DWORD extError = CommDlgExtendedError() )
  {
    String errors = "failed to save file because: ";
    // the user cancels or closes the Save dialog box
    // or an error such as the file name buffer being too small occurs
    switch( extError )
    {
      // the enums should be in commdlg.h, but its not finding, so fk it
      case 0xFFFF: errors += "CDERR_DIALOGFAILURE"; break;
      case 0x0006: errors += "CDERR_FINDRESFAILURE"; break;
      case 0x0002: errors += "CDERR_INITIALIZATION"; break;
      case 0x0007: errors += "CDERR_LOADRESFAILURE"; break;
      case 0x0005: errors += "CDERR_LOADSTRFAILURE"; break;
      case 0x0008: errors += "CDERR_LOCKRESFAILURE"; break;
      case 0x0009: errors += "CDERR_MEMALLOCFAILURE"; break;
      case 0x000A: errors += "CDERR_MEMLOCKFAILURE"; break;
      case 0x0004: errors += "CDERR_NOHINSTANCE"; break;
      case 0x000B: errors += "CDERR_NOHOOK"; break;
      case 0x0003: errors += "CDERR_NOTEMPLATE"; break;
      case 0x000C: errors += "CDERR_REGISTERMSGFAIL"; break;
      case 0x0001: errors += "CDERR_struct SIZE"; break;
      default: errors += "unknown"; break;
    }
    return errors;
  }

  // what if i inlined this into Win32DirectoryCallbackFunctor ?
  static void Win32DirectoryIterateAux( const WIN32_FIND_DATA& data,
                                        Win32DirectoryCallbackFunctor* fn,
                                        const Filesystem::Path& dir,
                                        Errors& errors )
  {
    const String dataFilename = data.cFileName;
    if( dataFilename == "." || dataFilename == ".." )
      return;
    ( *fn )( dir, data, errors );
  }

  static void Win32DirectoryIterate( const Filesystem::Path& dir,
                                     Win32DirectoryCallbackFunctor* fn,
                                     Errors& errors )
  {
    Filesystem::Path path = dir;
    path /= "/*";
    WIN32_FIND_DATA data;
    const HANDLE fileHandle = FindFirstFile( path.c_str(), &data );
    if( fileHandle == INVALID_HANDLE_VALUE )
    {
      const DWORD error = GetLastError();
      if( error == ERROR_FILE_NOT_FOUND )
        return;
      const String errMsg = Win32ErrorToString( error );
      TAC_RAISE_ERROR( errMsg, errors );
    }
    TAC_ON_DESTRUCT( FindClose( fileHandle ) );
    Win32DirectoryIterateAux( data, fn, dir, errors );
    TAC_HANDLE_ERROR( errors );
    while( FindNextFile( fileHandle, &data ) )
    {
      Win32DirectoryIterateAux( data, fn, dir, errors );
      TAC_HANDLE_ERROR( errors );
    }
    const DWORD error = GetLastError();
    if( error != ERROR_NO_MORE_FILES )
    {
      const String errMsg = Win32ErrorToString( error );
      TAC_RAISE_ERROR( errMsg, errors );
    }
  }

  static void Win32OSGetPrimaryMonitor( int* w, int* h )
  {
    *w = GetSystemMetrics( SM_CXSCREEN );
    *h = GetSystemMetrics( SM_CYSCREEN );
  }

  static void Win32OSGetWorkingDir( String& dir, Errors& errors )
  {
    const int bufLen = 1024;
    char buf[ bufLen ] = {};
    const DWORD getCurrentDirectoryResult = GetCurrentDirectory( bufLen,
                                                                 buf );
    if( 0 == getCurrentDirectoryResult )
    {
      const String errorMsg = Win32GetLastErrorString();
      TAC_RAISE_ERROR( errorMsg, errors );
    }
    dir = String( buf, getCurrentDirectoryResult );
  };

  static Filesystem::Path Win32OSOpenDialog(  Errors& errors )
  {
    const int outBufSize = 256;
    char outBuf[ outBufSize ] = {};
    DWORD flags = OFN_NOCHANGEDIR;

    OPENFILENAME dialogParams = {};
    dialogParams.lStructSize = sizeof( OPENFILENAME );
    dialogParams.lpstrFilter = "All files\0*.*\0\0";
    dialogParams.lpstrFile = outBuf;
    dialogParams.nMaxFile = outBufSize;
    dialogParams.Flags = flags;
    const BOOL getOpenFileNameResult = GetOpenFileNameA( &dialogParams );
    if( 0 == getOpenFileNameResult )
    {
      const DWORD extError = CommDlgExtendedError();
      if( 0 == extError )
        // The user closed/canceled the dialog box
        return {};

      const String errMsg = GetFileDialogErrors( extError );
      TAC_RAISE_ERROR_RETURN( errMsg, errors, {} );
    }

    Filesystem::Path path = outBuf;

    const Filesystem::Path workingDir = ShellGetInitialWorkingDir();
    if( path.starts_with( workingDir ) )
    {
      path = path.substr( workingDir.size() );
      path = Filesystem::StripLeadingSlashes( path );
    }
  }

  static Filesystem::Path Win32OSSaveDialog(  const Filesystem::Path& suggestedPath, Errors& errors )
  {
    Array< char, 256 > outBuf = {};
    MemCpy( outBuf.data(), suggestedPath.c_str(), suggestedPath.size() );

    // Prevent common file dialog from calling SetCurrentDirectory
    const DWORD flags = OFN_NOCHANGEDIR;

    OPENFILENAME dialogParams =
    {
      .lStructSize = sizeof( OPENFILENAME ),
      .lpstrFilter = "All files\0*.*\0\0",
      .lpstrFile = outBuf.data(),
      .nMaxFile = outBuf.size(),
      .Flags = flags,
    //.lpstrInitialDir = (LPCSTR)Shell::Instance.mPrefPath.c_str();
    };

    const BOOL saveResult = GetSaveFileNameA( &dialogParams );
    TAC_RAISE_ERROR_IF_RETURN( !saveResult, GetFileDialogErrors(), errors,{} );


    Filesystem::Path outPath = outBuf.data();
    return outPath;
  };

  static void Win32OSSetScreenspaceCursorPos( const v2& pos, Errors& errors )
  {
    TAC_RAISE_ERROR_IF( !SetCursorPos( ( int )pos.x, ( int )pos.y ), Win32GetLastErrorString(), errors );
  }

  static void* Win32OSGetLoadedDLL( StringView name )
  {
    HMODULE moduleHandle = GetModuleHandleA( name.c_str() );
    return moduleHandle;
  }

  static void* Win32OSLoadDLL( StringView path )
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

#if 0
  static void Win32OSDoesFolderExist( StringView path, bool& exists, Errors& errors )
  {
    String expandedPath;
    const char* pathBytes = path.c_str();

    const bool isFullPath = IsFullPath( path );
    if( !isFullPath )
    {
      String workingDir;
      Win32OSGetWorkingDir( workingDir, errors );
      expandedPath = workingDir + '\\' + String( path );
      pathBytes = expandedPath.c_str();
    }

    const DWORD dwAttrib = GetFileAttributes( pathBytes );
    if( dwAttrib == INVALID_FILE_ATTRIBUTES )
    {
      exists = false;
      return;
    }

    if( !( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) )
    {
      const String errMsg = path + "is not a directory";
      TAC_RAISE_ERROR( errMsg, errors );
    }

    exists = true;
  }

  static void Win32OSCreateFolder( const StringView path, Errors& errors )
  {
    bool exists;
    OS::OSDoesFolderExist(path, exists, errors);
    TAC_HANDLE_ERROR(errors);
    if (exists)
      return;


    const BOOL createDirectoryResult = CreateDirectoryA( path.c_str(), NULL );
    if( createDirectoryResult == 0 )
    {
      const String errMsg = "Failed to create folder at " + path + " because " + Win32GetLastErrorString();
      TAC_RAISE_ERROR( errMsg, errors );
    }
  }

  static void Win32OSSaveToFile( StringView path, const void* bytes, int byteCount, Errors& errors )
  {
    String directory = Filesystem::FilepathToDirectory( path );
    OS::OSCreateFolderIfNotExist( directory, errors );
    TAC_HANDLE_ERROR( errors );

    // Note ( from MSDN ):
    //   An application cannot create a directory by using CreateFile,
    //   To create a directory, the application must call CreateDirectory
    LPCSTR lpFileName = path.c_str();
    DWORD dwDesiredAccess = GENERIC_WRITE;
    DWORD dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL;

    // Note( n8 ) 2019-1-22:
    //   Changing from OPEN_ALWAYS to CREATE_ALWAYS,
    //   since OPEN_ALWAYS doesn't actually clear the file
    DWORD dwCreationDisposition = CREATE_ALWAYS;

    DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
    HANDLE hTemplateFile = NULL;
    const HANDLE handle = CreateFileA( lpFileName,
                                       dwDesiredAccess,
                                       dwShareMode,
                                       lpSecurityAttributes,
                                       dwCreationDisposition,
                                       dwFlagsAndAttributes,
                                       hTemplateFile );
    if( handle == INVALID_HANDLE_VALUE )
    {
      const String errMsg = "Cannot save to file " + String( path ) + " because " + Win32GetLastErrorString();
      TAC_RAISE_ERROR( errMsg, errors );
    }
    TAC_ON_DESTRUCT( CloseHandle( handle ) );
    DWORD bytesWrittenCount;
    if( !WriteFile( handle, bytes, byteCount, &bytesWrittenCount, NULL ) )
    {
      const String errMsg = "failed to save file " + String( path ) + " because " + Win32GetLastErrorString();
      TAC_RAISE_ERROR( errMsg, errors );
    }
    // Should we check that bytesWrittenCount == byteCount?
  }
#endif

  static void Win32OSDebugBreak()
  {
    Win32DebugBreak();
  }

  static void Win32OSDebugPopupBox( StringView s )
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
    Filesystem::Path roamingAppDataUTF8 = GetRoamingAppDataPathUTF8( errors );
    TAC_HANDLE_ERROR_RETURN( errors, {} );

    Filesystem::Path path;

    path = roamingAppDataUTF8;
    path /= ExecutableStartupInfo::sInstance.mStudioName;

#undef CreateDirectory

    Filesystem::CreateDirectory( path );
    const bool pathExists = Filesystem::Exists( path );
    TAC_ASSERT( pathExists );
    ++asdf;
    //OS::OSCreateFolder(path, errors);
    TAC_HANDLE_ERROR_RETURN( errors, {} );

    path /= ExecutableStartupInfo::sInstance.mAppName;
    const bool path2Exists = Filesystem::Exists( path );
    TAC_ASSERT( pathExists );
    ++asdf;
    //OS::OSCreateFolder(path, errors);
    TAC_HANDLE_ERROR_RETURN( errors, {} );
  }

  static void Win32OSGetFileLastModifiedTime( time_t* time,
                                  StringView path,
                                  Errors& errors )
  {
    // Path is allowed to be relative or full
    const HANDLE handle = CreateFile( path.c_str(),
                                      GENERIC_READ,
                                      FILE_SHARE_READ,
                                      0,
                                      OPEN_EXISTING,
                                      0,
                                      0 );
    if( handle == INVALID_HANDLE_VALUE )
    {
      const String errMsg = "Failed to open file " + path + " because " + Win32GetLastErrorString();
      TAC_RAISE_ERROR( errMsg, errors );
    }

    TAC_ON_DESTRUCT( CloseHandle( handle ) );
    BY_HANDLE_FILE_INFORMATION fileInfo;
    if( !GetFileInformationByHandle( handle, &fileInfo ) )
    {
      const String errMsg = Win32GetLastErrorString();
      TAC_RAISE_ERROR( errMsg, errors );
    }

    SYSTEMTIME lastWrite;
    if( !FileTimeToSystemTime( &fileInfo.ftLastWriteTime, &lastWrite ) )
    {
      const String errMsg = Win32GetLastErrorString();
      TAC_RAISE_ERROR( errMsg, errors );
    }

    tm tempTm = {};
    tempTm.tm_sec = lastWrite.wSecond;
    tempTm.tm_min = lastWrite.wMinute;
    tempTm.tm_hour = lastWrite.wHour;
    tempTm.tm_mday = lastWrite.wDay;
    tempTm.tm_mon = lastWrite.wMonth - 1;
    tempTm.tm_year = lastWrite.wYear - 1900;
    tempTm.tm_wday; // not needed for mktime
    tempTm.tm_yday; // not needed for mktime
    tempTm.tm_isdst = -1; // forget what this is for

    const time_t result = std::mktime( &tempTm );
    TAC_RAISE_ERROR_IF( result == -1, "Calandar time cannot be represented", errors );

    *time = result;
  }

  static Vector< String > Win32OSGetFilesInDirectory( const Filesystem::Path& dir,
                                                      OS::OSGetFilesInDirectoryFlags flags,
                                                      Errors& errors )
  {
    Vector< String > files;
    struct : public Win32DirectoryCallbackFunctor
    {
      void operator ()( const Filesystem::Path& dir,
                        const WIN32_FIND_DATA& data,
                        Errors& errors ) override
      {
        const bool isDirectory = data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        if( isDirectory )
        {
          if( mIsRecursive )
          {
            Filesystem::Path newPath = dir / data.cFileName;

            void operator ()( const Filesystem::Path& dir,
            Win32DirectoryIterate( newPath, this, errors );
          }
        }
        else
        {
          mFiles->push_back( dir + "/" + data.cFileName );
        }
      }

      bool              mIsRecursive = false;
      Vector< String >* mFiles = nullptr;
    } functor;

    functor.mIsRecursive = ( int )flags & ( int )OS::OSGetFilesInDirectoryFlags::Recursive;
    functor.mFiles = &files;

    Win32DirectoryIterate( dir, &functor, errors );
    return files;
  }

  static Vector< Filesystem::Path > Win32OSGetDirectoriesInDirectory( 
                                    const Filesystem::Path& dir,
                                    Errors& errors )
  {
    Vector< Filesystem::Path > dirs;
    struct : public Win32DirectoryCallbackFunctor
    {
      void operator ()( const Filesystem::Path& dir,
                        const WIN32_FIND_DATA& data,
                        Errors& errors ) override
      {
        TAC_UNUSED_PARAMETER( dir );
        TAC_UNUSED_PARAMETER( errors );
        if( !( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
          return;

        mDirs->push_back( data.cFileName );
      }
      Vector< Filesystem::Path >* mDirs = nullptr;
    } functor;
    functor.mDirs = &dirs;
    Win32DirectoryIterate( dir, &functor, errors );
    return dirs;
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
#if 0
    OS::OSSaveToFile = Win32OSSaveToFile;
    OS::OSDoesFolderExist = Win32OSDoesFolderExist;
    OS::OSCreateFolder = Win32OSCreateFolder;
#endif
    OS::OSDebugBreak = Win32OSDebugBreak;
    OS::OSDebugPopupBox = Win32OSDebugPopupBox;
    OS::OSGetApplicationDataPath = Win32OSGetApplicationDataPath;
    OS::OSGetFileLastModifiedTime = Win32OSGetFileLastModifiedTime;
    OS::OSGetFilesInDirectory = Win32OSGetFilesInDirectory;
    OS::OSGetDirectoriesInDirectory = Win32OSGetDirectoriesInDirectory;
    OS::OSSaveDialog = Win32OSSaveDialog;
    OS::OSOpenDialog = Win32OSOpenDialog;
    OS::OSGetWorkingDir = Win32OSGetWorkingDir;
    OS::OSGetPrimaryMonitor = Win32OSGetPrimaryMonitor;
    OS::OSSetScreenspaceCursorPos = Win32OSSetScreenspaceCursorPos;
    OS::OSGetLoadedDLL = Win32OSGetLoadedDLL;
    OS::OSLoadDLL = Win32OSLoadDLL;
  }
}

