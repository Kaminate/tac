#include "src/shell/windows/renderer/tac_renderer_directx.h"
#include "src/common/core/tac_preprocessor.h"
#include "src/common/system/tac_filesystem.h"
#include "src/common/memory/tac_frame_memory.h"
#include "src/common/system/tac_os.h"
#include "src/common/core/tac_error_handling.h"

//#include 	<libloaderapi.h>

namespace Tac::Render
{
  //static String ConvertUnsafe( LPWSTR p )
  //{
  //  String s;
  //  while( *p )
  //    s += (char)*p++;
  //  return s;
  //}

  static const char* pixInstallPath = "C:/Program Files/Microsoft PIX";
  static const char* pixDllName = "WinPixGpuCapturer.dll";


  static Filesystem::Paths GetInstalledPixVersions(Errors& errors)
  {
    const bool parentDirExist = Filesystem::Exists( pixInstallPath );
    //OS::OSDoesFolderExist( pixInstallPath, parentDirExist, errors );
    TAC_HANDLE_ERROR_RETURN( errors, {} );

    if( !parentDirExist )
      return {};

    const Filesystem::Path parentDirFSPath( pixInstallPath );

    const Filesystem::Paths subdirs = Filesystem::IterateDirectories( parentDirFSPath,
                                                                Filesystem::IterateType::Default,
                                                                errors );

    //OS::OSGetDirectoriesInDirectory( subdirs, pixInstallPath, errors );
    //TAC_HANDLE_ERROR_RETURN( errors, {} );

    return subdirs;
  }

  static String GetMostRecentVersion( const Filesystem::Paths& vers )
  {
    String mostRecentVersion;

    // Pix version numbers are usually XXXX.YY https://devblogs.microsoft.com/pix/download/
    for( const Filesystem::Path& subdir : vers )
    {
      const String ver = subdir.u8string();
      if( ver.size() != 6 && ver[ 4 ] != '.' )
        continue;

      if( mostRecentVersion.empty() || ver > mostRecentVersion )
        mostRecentVersion = ver;
    }

    return mostRecentVersion;
  }

  static String GetPIXDllPath(Errors& errors)
  {
    const Filesystem::Paths versions = GetInstalledPixVersions( errors );
    const String mostRecentVersion = GetMostRecentVersion( versions );
    if( mostRecentVersion.empty() )
      return "";

    String result;
    result += pixInstallPath;
    result += "/";
    result += mostRecentVersion;
    result += "/";
    result += pixDllName;
    return result;
  }

  void AllowPIXDebuggerAttachment( Errors& errors )
  {
    // Check to see if a copy of WinPixGpuCapturer.dll has already been injected into the application.
    // This may happen if the application is launched through the PIX UI. 
    if( OS::OSGetLoadedDLL( pixDllName ) )
      return;

    const String path = GetPIXDllPath( errors );
    if( path.empty() )
    {
        const char* str = FrameMemoryPrintf(
          "Warning: Could not find PIX %s. Is it installed? "
          "PIX will not be able to attach to the running process.",
          pixDllName );
        OS::OSDebugPrintLine( str );
        return;
    }

    void* lib = OS::OSLoadDLL( path.c_str() );
    if( !lib )
    {
      const char* str = FrameMemoryPrintf(
        "Failed to load PIX %s at path %s. Is the path correct?",
        pixDllName,
        path.c_str() );
      OS::OSDebugPrintLine( str );
    }

  }

} // namespace Tac::Render



