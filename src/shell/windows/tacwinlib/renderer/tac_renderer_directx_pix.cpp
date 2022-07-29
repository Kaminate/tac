#include "src/shell/windows/tacwinlib/renderer/tac_renderer_directx.h"
#include "src/common/tac_preprocessor.h"
#include "src/common/tac_os.h"

#include <shlobj.h> // SHGetKnownFolderPath

//import std.core;
import std.filesystem;

namespace Tac
{
  static std::string wstring_to_string( std::wstring s )
  {
    std::string result;
    for( auto c : s )
      result += ( char )c;
    return result;
  }

  static std::wstring GetLatestWinPixGpuCapturerPath_Cpp17()
  {
    LPWSTR programFilesPath = nullptr;
    SHGetKnownFolderPath( FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath );

    std::filesystem::path pixInstallationPath = programFilesPath;
    pixInstallationPath /= "Microsoft PIX";

    std::wstring newestVersionFound;

    bool directoryExists = std::filesystem::exists( pixInstallationPath );
    if( !directoryExists )
      return {};

    for( const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator( pixInstallationPath ) )
    {
      if( entry.is_directory() )
      {
        const std::filesystem::path& path = entry.path();
        const std::filesystem::path filename = path.filename();
        const std::wstring wstr = filename.wstring();
        if( newestVersionFound.empty() || newestVersionFound < wstr )
        {
          newestVersionFound = wstr;
        }
      }
    }

    TAC_ASSERT( !newestVersionFound.empty() );

    return pixInstallationPath / newestVersionFound / L"WinPixGpuCapturer.dll";
  }

  void AllowPIXDebuggerAttachment()
  {
    // Check to see if a copy of WinPixGpuCapturer.dll has already been injected into the application.
    // This may happen if the application is launched through the PIX UI. 
    HMODULE moduleHandle = GetModuleHandle( "WinPixGpuCapturer.dll" );
    if( moduleHandle )
      return;

    std::wstring wpath = GetLatestWinPixGpuCapturerPath_Cpp17();
    std::string path = wstring_to_string( wpath );
    HMODULE lib = LoadLibrary( path.c_str() );
    if( !lib )
      GetOS()->OSDebugPrintLine(
        "Warning: Could not find WinPixGpuCapturer.dll."
        " PIX (is it installed?) will not be attachable." );
  }

} // namespace Tac



