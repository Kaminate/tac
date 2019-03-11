#include "shell/windows/tacWindows.h"

#include "common/tacPreprocessor.h"
#include "common/tacUtility.h"
#include "common/tacOS.h"
#include "common/graphics/tacRenderer.h"
#include "common/tacAlgorithm.h"

#include <iostream>
#include <ctime> // mktime


#include <Shlobj.h> // SHGetKnownFolderPath

TacString TacWin32ErrorToString( DWORD winErrorValue )
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
  TacString result( ( LPCSTR )lpMsgBuf, bufLen );
  LocalFree( lpMsgBuf );
  //TacWindowsDebugBreak();
  return result;
}

TacString TacGetLastWin32ErrorString()
{
  DWORD winErrorValue = GetLastError();
  return TacWin32ErrorToString( winErrorValue );
}

TacString TacGetWin32WindowClass( HWND hwnd )
{
  const int byteCountIncNull = 100;
  char buffer[ byteCountIncNull ];
  GetClassNameA( hwnd, buffer, byteCountIncNull );
  TacString result = buffer;
  if( result == "#32768" ) return result + "(a menu)";
  if( result == "#32769" ) return result + "(the desktop window)";
  if( result == "#32770" ) return result + "(a dialog box)";
  if( result == "#32771" ) return result + "(the task switch window)";
  if( result == "#32772" ) return result + "(an icon titles)";
  return buffer;
}

TacString TacGetWin32WindowNameAux( HWND hwnd )
{
  const int byteCountIncNull = 100;
  char buffer[ byteCountIncNull ];
  GetWindowTextA( hwnd, buffer, byteCountIncNull );
  return buffer;
}

TacString TacGetWin32WindowName( HWND hwnd )
{
  TacString className = TacGetWin32WindowClass( hwnd );
  TacString windowName = TacGetWin32WindowNameAux( hwnd );
  return className + " " + windowName;
}

void TacWindowsAssert( TacErrors& errors )
{
  TacString s = errors.ToString();
  std::cout << s << std::endl;
  //s += "\n";
  //OutputDebugString( s.c_str() );
  TacWindowsDebugBreak();
  if( TacIsDebugMode() )
    return;
  MessageBox( NULL, s.c_str(), "Tac Assert", MB_OK );
  exit( -1 );
}

void TacWindowsDebugBreak()
{
  if( !TacIsDebugMode() )
    return;
  DebugBreak();
}

void TacWindowsPopup( TacString s )
{
  MessageBox( NULL, s.c_str(), "Message", MB_OK );
}

void TacWindowsOutput( TacString s )
{
  OutputDebugString( s.c_str() );
}


static TacString TacWideStringToUTF8( WCHAR* inputWideStr )
{
  TacString result;
  auto wCharCount = ( int )wcslen( inputWideStr );
  for( int i = 0; i < wCharCount; ++i )
  {
    result += ( char )inputWideStr[ i ];
  }
  return result;
}


struct TacWin32OS : public TacOS
{
  void GetScreenspaceCursorPos( v2& pos, TacErrors& errors ) override
  {
    POINT point;
    if( !GetCursorPos( &point ) )
    {
      errors += TacGetLastWin32ErrorString();
      TAC_HANDLE_ERROR( errors );
    }
    pos.x = ( float )point.x;
    pos.y = ( float )point.y;
  }
  void DoesFolderExist( const TacString& path, bool& exists, TacErrors& errors ) override
  {
    DWORD dwAttrib = GetFileAttributes( path.c_str() );
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
  void CreateFolder( const TacString& path, TacErrors& errors ) override
  {
    BOOL createDirectoryResult = CreateDirectoryA( path.c_str(), NULL );
    if( createDirectoryResult == 0 )
    {
      errors = "Failed to create folder at " + path + " because " + TacGetLastWin32ErrorString();
      TAC_HANDLE_ERROR( errors );
    }
  }
  void SaveToFile( const TacString& path, void* bytes, int byteCount, TacErrors& errors ) override
  {
    TacSplitFilepath splitFilepath( path );
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
      errors = "Cannot save to file " + path + " because " + TacGetLastWin32ErrorString();
      TAC_HANDLE_ERROR( errors );
    }
    OnDestruct( CloseHandle( handle ) );
    DWORD bytesWrittenCount;
    if( !WriteFile( handle, bytes, byteCount, &bytesWrittenCount, NULL ) )
    {
      errors = "failed to save file " + path + " because " + TacGetLastWin32ErrorString();
      TAC_HANDLE_ERROR( errors );
    }
    // Should we check that bytesWrittenCount == byteCount?
  }
  void DebugBreak() override
  {
    TacWindowsDebugBreak();
  }
  void DebugPopupBox( const TacString& s ) override
  {
    MessageBox( nullptr, s.data(), nullptr, MB_OK );
  }
  void GetApplicationDataPath( TacString& path, TacErrors& errors ) override
  {
    WCHAR* outPath;
    HRESULT hr = SHGetKnownFolderPath( FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &outPath );
    if( hr != S_OK )
    {
      errors += "Failed to get roaming folder";
      TAC_HANDLE_ERROR( errors );
    }
    path = TacWideStringToUTF8( outPath );
    CoTaskMemFree( outPath );
  }
  TacString GetDefaultRendererName() override
  {
    return RendererNameDirectX11;
  }
  void GetFileLastModifiedTime( time_t* time, const TacString& path, TacErrors& errors ) override
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
      errors += TacGetLastWin32ErrorString();
      TAC_HANDLE_ERROR( errors );
    }

    SYSTEMTIME lastWrite;
    if( !FileTimeToSystemTime( &fileInfo.ftLastWriteTime, &lastWrite ) )
    {
      errors += TacGetLastWin32ErrorString();
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
    TacVector<TacString>&files,
    const TacString& dir,
    TacErrors& errors )
  {
    TacString dataFilename = data.cFileName;
    if( dataFilename == "." || dataFilename == ".." )
      return;
    TacString dataFilepath = dir + "/" + dataFilename;
    if( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
    {
      GetDirFilesRecrusive( files, dataFilepath, errors );
      TAC_HANDLE_ERROR( errors );
    }
    else
    {
      files.push_back( dataFilepath );
    }
  }
  void GetDirFilesRecrusive( TacVector<TacString>&files, const TacString& dir, TacErrors& errors ) override
  {
    TacString path = dir + "/*";
    WIN32_FIND_DATA data;
    HANDLE fileHandle = FindFirstFile( path.c_str(), &data );
    if( fileHandle == INVALID_HANDLE_VALUE )
    {
      DWORD error = GetLastError();
      if( error != ERROR_FILE_NOT_FOUND )
      {
        errors += TacWin32ErrorToString( error );
        TAC_HANDLE_ERROR( errors );
      }
    }
    OnDestruct( FindClose( fileHandle ) );
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
      errors += TacWin32ErrorToString( error );
      TAC_HANDLE_ERROR( errors );
    }
  }
};


int setOSInstance = []()
{
  TacOS::Instance = new TacWin32OS;
  return 0;
}( );
