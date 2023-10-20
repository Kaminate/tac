#include "src/shell/windows/renderer/tac_renderer_directx.h"
#include "src/common/core/tac_preprocessor.h"
#include "src/common/memory/tac_frame_memory.h"
#include "src/common/system/tac_os.h"
#include "src/common/core/tac_error_handling.h"

//#include 	<libloaderapi.h>

namespace Tac
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

  static String GetPIXDllPath(Errors& errors)
  {
    bool parentDirExist = false;
    OS::OSDoesFolderExist( pixInstallPath, parentDirExist, errors );
    TAC_HANDLE_ERROR_RETURN( errors, "" );

    if( !parentDirExist )
      return "";

    Vector< String > subdirs;
    OS::OSGetDirectoriesInDirectory( subdirs, pixInstallPath, errors );
    TAC_HANDLE_ERROR_RETURN( errors, "" );

    String mostRecentVersion;

    // Pix version numbers are usually XXXX.YY https://devblogs.microsoft.com/pix/download/
    for( const String& subdir : subdirs )
    {
      if( subdir.size() != 6 && subdir[ 4 ] != '.' )
        continue;

      if( mostRecentVersion.empty() || subdir > mostRecentVersion )
        mostRecentVersion = subdir;
    }

    std::vector< std::string > vers = { "2305.10" , "2303.30","2303.02","2208.10" };
    std::string v = vers[ 0 ];
    std::string_view sv = v;

    Vector<int> tac_vec = { 1,2,3 };
    StringView tacsv = mostRecentVersion;

    TAC_ASSERT( !mostRecentVersion.empty());
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

    String path = GetPIXDllPath( errors );
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

} // namespace Tac



