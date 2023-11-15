#include "src/shell/windows/renderer/pix/tac_pix.h" // self-inc

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
      const String ver = subdir.dirname().u8string();

      //auto parent = subdir.parent_path();
      //std::filesystem::path p =  subdir.Get();

      //auto rootDir = p.root_directory();
      //auto rootName = p.root_name();
      //auto rootPath = p.root_path();
      //auto relPath = p.relative_path();
      //auto fname = p.filename();
      //auto stem = p.stem();
      //auto extension = p.extension();


      //const String ver = subdir.u8string();
      if( ver.size() != 6 && ver[ 4 ] != '.' )
        continue;

      if( mostRecentVersion.empty() || ver > mostRecentVersion )
        mostRecentVersion = ver;
    }

    if( !vers.empty() )
    {
      TAC_ASSERT(!mostRecentVersion.empty());
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
        OS::OSDebugPrintLine( va(
          "Warning: Could not find PIX {}. Is it installed? "
          "PIX will not be able to attach to the running process.",
          pixDllName ) );
        return;
    }

    void* lib = OS::OSLoadDLL( path.c_str() );
    if( !lib )
    {
      OS::OSDebugPrintLine( va(
        "Failed to load PIX {} at path {}. Is the path correct?",
        pixDllName,
        path.c_str() ) );
    }

  }

} // namespace Tac::Render



