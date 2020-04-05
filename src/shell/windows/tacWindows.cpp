#include "src/shell/windows/tacWindows.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacUtility.h"
#include "src/common/tacOS.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacShell.h"

#include <iostream>
#include <ctime> // mktime


#include <Shlobj.h> // SHGetKnownFolderPath


#include <commdlg.h> // GetSaveFileNameA
#pragma comment( lib, "Comdlg32.lib" ) // GetSaveFileNameA


namespace Tac
{



String Win32ErrorToString( DWORD winErrorValue )
{
  if( !winErrorValue )
    return "no error";
  LPVOID lpMsgBuf;
  DWORD bufLen = FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER |
    FORMAT_MESSAGE_FROM_SYSTEM |
    FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    winErrorValue,
    MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
    ( LPTSTR )&lpMsgBuf,
    0,
    NULL );
  if( !bufLen )
    return "FormatMessage() failed";
  String result( ( LPCSTR )lpMsgBuf, bufLen );
  LocalFree( lpMsgBuf );
  //WindowsDebugBreak();
  return result;
}

String GetLastWin32ErrorString()
{
  DWORD winErrorValue = GetLastError();
  return Win32ErrorToString( winErrorValue );
}

String GetWin32WindowClass( HWND hwnd )
{
  const int byteCountIncNull = 100;
  char buffer[ byteCountIncNull ];
  GetClassNameA( hwnd, buffer, byteCountIncNull );
  String result = buffer;
  if( result == "#32768" ) return result + "(a menu)";
  if( result == "#32769" ) return result + "(the desktop window)";
  if( result == "#32770" ) return result + "(a dialog box)";
  if( result == "#32771" ) return result + "(the task switch window)";
  if( result == "#32772" ) return result + "(an icon titles)";
  return buffer;
}

String GetWin32WindowNameAux( HWND hwnd )
{
  const int byteCountIncNull = 100;
  char buffer[ byteCountIncNull ];
  GetWindowTextA( hwnd, buffer, byteCountIncNull );
  return buffer;
}

String GetWin32WindowName( HWND hwnd )
{
  String className = GetWin32WindowClass( hwnd );
  String windowName = GetWin32WindowNameAux( hwnd );
  return className + " " + windowName;
}

void WindowsAssert( Errors& errors )
{
  String s = errors.ToString();
  std::cout << s << std::endl;
  //s += "\n";
  //OutputDebugString( s.c_str() );
  WindowsDebugBreak();
  if( IsDebugMode() )
    return;
  MessageBox( NULL, s.c_str(), "Tac Assert", MB_OK );
  exit( -1 );
}

void WindowsDebugBreak()
{
  if( !IsDebugMode() )
    return;
  DebugBreak();
}

void WindowsPopup( String s )
{
  MessageBox( NULL, s.c_str(), "Message", MB_OK );
}

void WindowsOutput( String s )
{
  OutputDebugString( s.c_str() );
}


static String WideStringToUTF8( WCHAR* inputWideStr )
{
  String result;
  auto wCharCount = ( int )wcslen( inputWideStr );
  for( int i = 0; i < wCharCount; ++i )
  {
    result += ( char )inputWideStr[ i ];
  }
  return result;
}

static String GetFileDialogErrors()
{
  String errors;

  errors = "failed to save file";
  // the user cancels or closes the Save dialog box
  // or an error such as the file name buffer being too small occurs
  DWORD extError = CommDlgExtendedError();
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

struct Win32OS : public OS
{

  void GetWorkingDir( String& dir, Errors& errors ) override
  {
    const int bufLen = 1024;
    char buf[ bufLen ] = {};
    DWORD getCurrentDirectoryResult = GetCurrentDirectory(
      bufLen,
      buf );
    if( 0 == getCurrentDirectoryResult )
    {
      errors = GetLastWin32ErrorString();
      return;
    }
    dir = String( buf, getCurrentDirectoryResult );
  };
  void OpenDialog( String& path, Errors& errors ) override
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
    BOOL getOpenFileNameResult = GetOpenFileNameA( &dialogParams );
    if( 0 == getOpenFileNameResult )
    {
      errors = GetFileDialogErrors();
      return;
    }

    path = outBuf;

    const String& workingDir = Shell::Instance->mInitialWorkingDir;
    if( StartsWith( path, workingDir ) )
    {
      path = path.substr( workingDir.size() );
      path = StripLeadingSlashes( path );
    }
  }
  void SaveDialog( String& path, const String& suggestedPath, Errors& errors ) override
  {
    //IFileDialog *pfd = NULL;
    //// CoCreate the File Open Dialog object.
    //HRESULT hr = CoCreateInstance(
    //  CLSID_FileSaveDialog,
    //  NULL,
    //  CLSCTX_INPROC_SERVER,
    //  IID_PPV_ARGS( &pfd ) );
    //if( FAILED( hr ) )
    //{
    //  errors = "cocreateinstance failed";
    //  return;
    //}
    //// Create an event handling object, and hook it up to the dialog.
    //IFileDialogEvents *pfde = NULL;
    //hr = CDialogEventHandler_CreateInstance( IID_PPV_ARGS( &pfde ) );
    // ^ this is a helper fn from a windows sample...

    const int outBufSize = 256;
    char outBuf[ outBufSize ] = {};
    MemCpy( outBuf, suggestedPath.c_str(), suggestedPath.size() );

    // https://devblogs.microsoft.com/oldnewthing/20101112-00/?p=12293
    // When you change folders in a common file dialog,
    // the common file dialog calls Set­Current­Directory to match the
    // directory you are viewing.
    //
    // Many programs require that the current directory match the
    // directory containing the document being opened.
    //
    // OFN_NOCHANGEDIR avoids this ( doesn't work on GetOpenFileName )
    DWORD flags = OFN_NOCHANGEDIR;

    OPENFILENAME dialogParams = {};
    dialogParams.lStructSize = sizeof( OPENFILENAME );
    dialogParams.lpstrFilter = "All files\0*.*\0\0";
    dialogParams.lpstrFile = outBuf;
    dialogParams.nMaxFile = outBufSize;
    dialogParams.Flags = flags;


    // | actually, no.
    // v we want the prefabs to be saved in the install ( working ) dir
    // dialogParams.lpstrInitialDir = (LPCSTR)Shell::Instance->mPrefPath.c_str();

    BOOL getSaveFileNameResult = GetSaveFileNameA( &dialogParams );
    if( 0 == getSaveFileNameResult )
    {
      errors = GetFileDialogErrors();
      return;
    }

    path = outBuf;
  };
  void GetScreenspaceCursorPos( v2& pos, Errors& errors ) override
  {
    // Note: this could return error access denied, for example if your computer goes to sleep
    POINT point;
    if( !GetCursorPos( &point ) )
    {
      errors += GetLastWin32ErrorString();
      TAC_HANDLE_ERROR( errors );
    }
    pos.x = ( float )point.x;
    pos.y = ( float )point.y;
  }
  void SetScreenspaceCursorPos( v2& pos, Errors& errors ) override
  {
    if( SetCursorPos( ( int )pos.x, ( int )pos.y ) )
      return;
    errors += GetLastWin32ErrorString();
    TAC_HANDLE_ERROR( errors );
  }
  void DoesFolderExist( const String& path, bool& exists, Errors& errors ) override
  {
    String expandedPath;
    const char* pathBytes = path.c_str();

    bool isFullPath =
      IsAlpha( path[ 0 ] ) &&
      ':' == path[ 1 ] &&
      '\\' == path[ 2 ];
    if( !isFullPath )
    {
      String workingDir;
      GetWorkingDir( workingDir, errors );
      expandedPath = workingDir + '\\' + path;
      pathBytes = expandedPath.c_str();
    }

    DWORD dwAttrib = GetFileAttributes( pathBytes );
    if( dwAttrib == INVALID_FILE_ATTRIBUTES )
    {
      exists = false;
      return;
    }
    if( !( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) )
    {
      errors = path + "is not a directory";
      return;
    }
    exists = true;
  }
  void CreateFolder( const String& path, Errors& errors ) override
  {
    BOOL createDirectoryResult = CreateDirectoryA( path.c_str(), NULL );
    if( createDirectoryResult == 0 )
    {
      errors = "Failed to create folder at " + path + " because " + GetLastWin32ErrorString();
      TAC_HANDLE_ERROR( errors );
    }
  }
  void SaveToFile( const String& path, void* bytes, int byteCount, Errors& errors ) override
  {
    SplitFilepath splitFilepath( path );
    CreateFolderIfNotExist( splitFilepath.mDirectory, errors );
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
    HANDLE handle = CreateFileA(
      lpFileName,
      dwDesiredAccess,
      dwShareMode,
      lpSecurityAttributes,
      dwCreationDisposition,
      dwFlagsAndAttributes,
      hTemplateFile );
    if( handle == INVALID_HANDLE_VALUE )
    {
      errors = "Cannot save to file " + path + " because " + GetLastWin32ErrorString();
      TAC_HANDLE_ERROR( errors );
    }
    TAC_ON_DESTRUCT( CloseHandle( handle ) );
    DWORD bytesWrittenCount;
    if( !WriteFile( handle, bytes, byteCount, &bytesWrittenCount, NULL ) )
    {
      errors = "failed to save file " + path + " because " + GetLastWin32ErrorString();
      TAC_HANDLE_ERROR( errors );
    }
    // Should we check that bytesWrittenCount == byteCount?
  }
  void DebugBreak() override
  {
    WindowsDebugBreak();
  }
  void DebugPopupBox( const String& s ) override
  {
    MessageBox( nullptr, s.data(), nullptr, MB_OK );
  }
  void GetApplicationDataPath( String& path, Errors& errors ) override
  {
    WCHAR* outPath;
    HRESULT hr = SHGetKnownFolderPath( FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &outPath );
    if( hr != S_OK )
    {
      errors += "Failed to get roaming folder";
      TAC_HANDLE_ERROR( errors );
    }
    path = WideStringToUTF8( outPath );
    CoTaskMemFree( outPath );
  }
  String GetDefaultRendererName() override
  {
    return RendererNameDirectX11;
  }
  void GetFileLastModifiedTime( time_t* time, const String& path, Errors& errors ) override
  {
    HANDLE handle = CreateFile(
      path.c_str(),
      OPEN_EXISTING,
      FILE_SHARE_READ,
      0,
      GENERIC_READ,
      0,
      0 );
    if( handle == INVALID_HANDLE_VALUE )
    {
      errors = "Failed to open file to get last modified time " + path;
      TAC_HANDLE_ERROR( errors );
    }
    BY_HANDLE_FILE_INFORMATION fileInfo;
    if( !GetFileInformationByHandle( handle, &fileInfo ) )
    {
      errors += GetLastWin32ErrorString();
      TAC_HANDLE_ERROR( errors );
    }

    SYSTEMTIME lastWrite;
    if( !FileTimeToSystemTime( &fileInfo.ftLastWriteTime, &lastWrite ) )
    {
      errors += GetLastWin32ErrorString();
      TAC_HANDLE_ERROR( errors );
    }

    tm tempTm;
    tempTm.tm_sec = lastWrite.wSecond;
    tempTm.tm_min = lastWrite.wMinute;
    tempTm.tm_hour = lastWrite.wHour;
    tempTm.tm_mday = lastWrite.wDay;
    tempTm.tm_mon = lastWrite.wMonth - 1;
    tempTm.tm_year = lastWrite.wYear - 1900;
    tempTm.tm_wday; // not needed for mktime
    tempTm.tm_yday; // not needed for mktime
    tempTm.tm_isdst = -1; // forget what this is for

    time_t result = std::mktime( &tempTm );
    if( result == -1 )
    {
      errors = "Calandar time cannot be represented";
      TAC_HANDLE_ERROR( errors );
    }

    *time = result;
  }
  void GetDirFilesRecrusiveAux(
    const WIN32_FIND_DATA& data,
    Vector<String>&files,
    const String& dir,
    Errors& errors )
  {
    String dataFilename = data.cFileName;
    if( dataFilename == "." || dataFilename == ".." )
      return;
    String dataFilepath = dir + "/" + dataFilename;
    if( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
    {
      GetDirFilesRecursive( files, dataFilepath, errors );
      TAC_HANDLE_ERROR( errors );
    }
    else
    {
      files.push_back( dataFilepath );
    }
  }
  void GetDirFilesRecursive( Vector<String>&files, const String& dir, Errors& errors ) override
  {
    String path = dir + "/*";
    WIN32_FIND_DATA data;
    HANDLE fileHandle = FindFirstFile( path.c_str(), &data );
    if( fileHandle == INVALID_HANDLE_VALUE )
    {
      DWORD error = GetLastError();
      if( error != ERROR_FILE_NOT_FOUND )
      {
        errors += Win32ErrorToString( error );
        TAC_HANDLE_ERROR( errors );
      }
    }
    TAC_ON_DESTRUCT( FindClose( fileHandle ) );
    GetDirFilesRecrusiveAux( data, files, dir, errors );
    TAC_HANDLE_ERROR( errors );
    while( FindNextFile( fileHandle, &data ) )
    {
      GetDirFilesRecrusiveAux( data, files, dir, errors );
      TAC_HANDLE_ERROR( errors );
    }
    DWORD error = GetLastError();
    if( error != ERROR_NO_MORE_FILES )
    {
      errors += Win32ErrorToString( error );
      TAC_HANDLE_ERROR( errors );
    }
  }
};


int setOSInstance = []()
{
  OS::Instance = new Win32OS;
  return 0;
}( );
}
