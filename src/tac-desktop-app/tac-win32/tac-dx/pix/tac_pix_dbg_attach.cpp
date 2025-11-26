#include "tac_pix_dbg_attach.h" // self-inc

#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac::Render
{
  static const UTF8Path pixInstallPath{ "C:/Program Files/Microsoft PIX" };
  static const char* pixDllName{ "WinPixGpuCapturer.dll" };

  // Looking at https://devblogs.microsoft.com/pix/download/
  // Pix version numbers seem to usually be XXXX.YY 
  static bool IsVersionPattern( StringView sv )
  {
    if( sv.size() != 7 )
      return false;

    const char* p{ sv.data() };

    for( int i{}; i < 4; ++i )
      if( !IsDigit( *p++ ) )
        return false;

    if( '.' != *p++ )
      return false;

    for( int i{}; i < 2; ++i )
      if( !IsDigit( *p++ ) )
        return false;

    return true;
  }

  static auto TryFindPIXDllPath( Errors& errors ) -> UTF8Path
  {
    if( !pixInstallPath.Exists() )
      return {};

    TAC_CALL_RET( const UTF8Paths subdirs{ pixInstallPath.IterateDirectories(
      UTF8Path::IterateType::Default,
      errors ) } );

    if( subdirs.empty() )
      return {};

    String bestVer;
    for( const UTF8Path& subdir : subdirs )
    {
      const String ver { subdir.dirname() };
      if( !IsVersionPattern( ver ) || ver < bestVer )// ( !bestSubdir.empty() && ver < bestSubdir ) )
        continue;

      bestVer = ver;
    }

    if( !bestVer.empty() )
    {
      const String minVer { "2312.08"  };
      TAC_RAISE_ERROR_IF_RETURN( bestVer < minVer,
                                 "Most recent pix version " + bestVer + " "
                                 "is below min required PIX ver " + minVer + " "
                                 "please update PIX" );
      return pixInstallPath / bestVer / pixDllName;
    }
    
    return subdirs.front() / pixDllName;
  }

  void AllowPIXDebuggerAttachment( Errors& errors )
  {
    if constexpr( kIsDebugMode )
    {
      //if( !OS::CmdLineIsFlagPresent( "allowpixattach" ) )
      //  return;

      // Check to see if a copy of WinPixGpuCapturer.dll has already been injected into the application.
      // This may happen if the application is launched through the PIX UI. 
      if( OS::OSGetLoadedDLL( pixDllName ) )
        return;

      const UTF8Path path{ TryFindPIXDllPath( errors ) };
      const String path8{ path };
      if( path8.empty() )
      {
        OS::OSDebugPrintLine( String() + "Warning: Could not find PIX dll " + pixDllName
                              + ". Is it installed? "
                              "PIX will not be able to attach to the running process." );
        return;
      }

      
      if( void* lib{ OS::OSLoadDLL( path ) };
          !lib )
      {
        OS::OSDebugPrintLine( String() +
                              "Failed to load PIX dll " + pixDllName + " at path " + path8 + "." );
      }
    }
  }

} // namespace Tac::Render



