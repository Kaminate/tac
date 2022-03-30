#include "src/common/containers/tac_array.h"
#include "src/common/containers/tac_fixed_vector.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/tac_algorithm.h"
#include "src/common/tac_id_collection.h"
#include "src/common/tac_os.h"
#include "src/common/tac_preprocessor.h"
#include "src/common/tac_utility.h"
#include "src/shell/windows/tacwinlib/tac_win32.h"

#include <iostream>
#include <ctime> // mktime
#include <Shlobj.h> // SHGetKnownFolderPath
#include <commdlg.h> // GetSaveFileNameA
#pragma comment( lib, "Comdlg32.lib" ) // GetSaveFileNameA

namespace Tac
{
  struct Win32DirectoryCallbackFunctor
  {
    virtual void operator()( StringView dir,
                             const WIN32_FIND_DATA&,
                             Errors& ) = 0;
  };

  static String WideStringToUTF8( WCHAR* inputWideStr )
  {
    String result;
    auto wCharCount = ( int )wcslen( inputWideStr );
    for( int i = 0; i < wCharCount; ++i )
      result += ( char )inputWideStr[ i ];
    return result;
  }

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
                                        StringView dir,
                                        Errors& errors )
  {
    const String dataFilename = data.cFileName;
    if( dataFilename == "." || dataFilename == ".." )
      return;
    ( *fn )( dir, data, errors );
  }

  static void Win32DirectoryIterate( StringView dir,
                                     Win32DirectoryCallbackFunctor* fn,
                                     Errors& errors )
  {
    const String path = dir + "/*";
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

  void Win32OSGetPrimaryMonitor( int* w, int* h )
  {
    *w = GetSystemMetrics( SM_CXSCREEN );
    *h = GetSystemMetrics( SM_CYSCREEN );
  }

  void Win32OSGetWorkingDir( String& dir, Errors& errors )
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

  void Win32OSOpenDialog( String& path, Errors& errors )
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
        return;

      const String errMsg = GetFileDialogErrors( extError );
      TAC_RAISE_ERROR( errMsg, errors );
    }

    path = outBuf;

    const StringView workingDir = ShellGetInitialWorkingDir();
    if( StartsWith( path, workingDir ) )
    {
      path = path.substr( workingDir.size() );
      path = StripLeadingSlashes( path );
    }
  }

  void Win32OSSaveDialog( String& outPath, StringView suggestedPath, Errors& errors )
  {
    Array< char, 256 > outBuf = {};
    MemCpy( outBuf.data(), suggestedPath.c_str(), suggestedPath.size() );

    // Prevent common file dialog from calling SetCurrentDirectory
    const DWORD flags = OFN_NOCHANGEDIR;

    OPENFILENAME dialogParams = {};
    dialogParams.lStructSize = sizeof( OPENFILENAME );
    dialogParams.lpstrFilter = "All files\0*.*\0\0";
    dialogParams.lpstrFile = outBuf.data();
    dialogParams.nMaxFile = outBuf.size();
    dialogParams.Flags = flags;
    // dialogParams.lpstrInitialDir = (LPCSTR)Shell::Instance.mPrefPath.c_str();

    if( !GetSaveFileNameA( &dialogParams ) )
    {
      const String errMsg = GetFileDialogErrors();
      TAC_RAISE_ERROR( errMsg, errors );
    }

    outPath = outBuf.data();
  };

  void Win32OSSetScreenspaceCursorPos( const v2& pos, Errors& errors )
  {
    TAC_RAISE_ERROR_IF( !SetCursorPos( ( int )pos.x, ( int )pos.y ), Win32GetLastErrorString(), errors );
  }

  void Win32OSDoesFolderExist( StringView path, bool& exists, Errors& errors )
  {
    String expandedPath;
    const char* pathBytes = path.c_str();

    bool isFullPath = IsAlpha( path[ 0 ] ) &&
      ':' == path[ 1 ] &&
      '\\' == path[ 2 ];
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

  void Win32OSCreateFolder( const StringView path, Errors& errors )
  {
    const BOOL createDirectoryResult = CreateDirectoryA( path.c_str(), NULL );
    if( createDirectoryResult == 0 )
    {
      const String errMsg = "Failed to create folder at " + path + " because " + Win32GetLastErrorString();
      TAC_RAISE_ERROR( errMsg, errors );
    }
  }

  void Win32OSSaveToFile( StringView path, void* bytes, int byteCount, Errors& errors )
  {
    SplitFilepath splitFilepath( path );
    GetOS()->OSCreateFolderIfNotExist( splitFilepath.mDirectory, errors );
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

  void Win32OSDebugBreak()
  {
    Win32DebugBreak();
  }

  void Win32OSDebugPopupBox( StringView s )
  {
    MessageBox( nullptr, s.data(), nullptr, MB_OK );
  }

  void Win32OSGetApplicationDataPath( String& path, Errors& errors )
  {
    WCHAR* outPath;
    const HRESULT hr = SHGetKnownFolderPath( FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &outPath );
    if( hr != S_OK )
    {
      TAC_RAISE_ERROR( "Failed to get roaming folder", errors );
    }
    path = WideStringToUTF8( outPath );
    CoTaskMemFree( outPath );
  }

  void Win32OSGetFileLastModifiedTime( time_t* time,
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

  void Win32OSGetFilesInDirectory( Vector< String >& files,
                              StringView dir,
                              OSGetFilesInDirectoryFlags flags,
                              Errors& errors )
  {
    struct : public Win32DirectoryCallbackFunctor
    {
      void operator ()( StringView dir,
                        const WIN32_FIND_DATA& data,
                        Errors& errors ) override
      {
        const bool isDirectory = data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        if( isDirectory )
        {
          if( mIsRecursive )
          {
            Win32DirectoryIterate( dir + "/" + data.cFileName, this, errors );
          }
        }
        else
        {
          mFiles->push_back( dir + "/" + data.cFileName );
        }
      }

      bool              mIsRecursive;
      Vector< String >* mFiles;
    } functor;

    functor.mIsRecursive = ( int )flags & ( int )OSGetFilesInDirectoryFlags::Recursive;
    functor.mFiles = &files;

    Win32DirectoryIterate( dir, &functor, errors );
  }

  void Win32OSGetDirectoriesInDirectory( Vector< String >& dirs,
                                    StringView dir,
                                    Errors& errors )
  {
    struct : public Win32DirectoryCallbackFunctor
    {
      void operator ()( StringView dir,
                        const WIN32_FIND_DATA& data,
                        Errors& errors ) override
      {
        TAC_UNUSED_PARAMETER( dir );
        TAC_UNUSED_PARAMETER( errors );
        if( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
          mDirs->push_back( data.cFileName );
      }
      Vector< String >* mDirs;
    } functor;
    functor.mDirs = &dirs;
    Win32DirectoryIterate( dir, &functor, errors );
  }

  static const int    kSemaphoreCapacity = 10;
  static IdCollection gSemaphoreIds( kSemaphoreCapacity );
  static HANDLE       gSemaphores[ kSemaphoreCapacity ];

  SemaphoreHandle Win32OSSemaphoreCreate()
  {
    const int i = gSemaphoreIds.Alloc();
    HANDLE semaphore = CreateSemaphoreA( NULL, 0, 100, NULL );
    TAC_ASSERT( semaphore );
    gSemaphores[ i ] = semaphore;
    return i;
  }

  void            Win32OSSemaphoreDecrementWait( const SemaphoreHandle handle )
  {
    TAC_ASSERT( handle.IsValid() );
    const HANDLE nativeHandle = gSemaphores[ ( int )handle ];
    WaitForSingleObject( nativeHandle, INFINITE );
  }

  void            Win32OSSemaphoreIncrementPost( const SemaphoreHandle handle )
  {
    TAC_ASSERT( handle.IsValid() );
    const HANDLE nativeHandle = gSemaphores[ ( int )handle ];
    ReleaseSemaphore( nativeHandle, 1, NULL );
  }


  static struct : public OS
  {
    void        OSSaveToFile( StringView path, void* bytes, int byteCount, Errors&  errors ) override
    {
      return Win32OSSaveToFile( path, bytes, byteCount, errors );
    }

    void        OSDoesFolderExist( StringView path, bool& exists, Errors& errors ) override
    {
      return Win32OSDoesFolderExist( path, exists, errors );
    }

    void        OSCreateFolder( StringView path, Errors& errors ) override
    {
      return Win32OSCreateFolder( path, errors );
    }

    void        OSDebugBreak() override { Win32OSDebugBreak(); }

    void        OSDebugPopupBox( StringView s ) override { return Win32OSDebugPopupBox( s ); }

    void        OSGetApplicationDataPath( String& path, Errors& e ) override
    {
      return Win32OSGetApplicationDataPath( path, e );
    }

    void        OSGetFileLastModifiedTime( time_t* t, StringView path, Errors& e ) override
    {
      return Win32OSGetFileLastModifiedTime( t, path, e );
    }

    void        OSGetFilesInDirectory( Vector< String >& files,
                                       StringView dir,
                                       OSGetFilesInDirectoryFlags flags,
                                       Errors& e )
    {
      return Win32OSGetFilesInDirectory( files, dir, flags, e );
    }

    void        OSGetDirectoriesInDirectory( Vector< String >& dirs, StringView dir, Errors& e ) override
    {
      return Win32OSGetDirectoriesInDirectory( dirs, dir, e );
    }

    void        OSSaveDialog( String& path, StringView suggestedPath, Errors& e ) override
    {
      return Win32OSSaveDialog( path, suggestedPath, e );
    }

    void        OSOpenDialog( String& path, Errors& e ) override
    {
      return Win32OSOpenDialog( path, e );
    }


    //                  same as current dir
    void        OSGetWorkingDir( String& dir, Errors& e ) override
    {
      return Win32OSGetWorkingDir( dir, e );
    }


    void        OSGetPrimaryMonitor( int* w, int* h ) override
    {
      return Win32OSGetPrimaryMonitor( w, h );
    }

    void        OSSetScreenspaceCursorPos( const v2& v, Errors& e ) override
    {
      return Win32OSSetScreenspaceCursorPos( v, e );
    }




    SemaphoreHandle OSSemaphoreCreate() override
    {
      return Win32OSSemaphoreCreate();
    }

    void            OSSemaphoreDecrementWait( SemaphoreHandle handle ) override
    {
      return Win32OSSemaphoreDecrementWait( handle );
    }

    void            OSSemaphoreIncrementPost( SemaphoreHandle handle ) override
    {
      return Win32OSSemaphoreIncrementPost( handle );
    }


  } sWin32OS;

  void             Win32OSInit()
  {
    SetOS( &sWin32OS );
  }
}

