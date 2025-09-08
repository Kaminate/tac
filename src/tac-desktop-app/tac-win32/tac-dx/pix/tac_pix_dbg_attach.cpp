#include "tac_pix_dbg_attach.h" // self-inc

#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/error/tac_error_handling.h"

#define TRY_SET_HUD_OPTIONS() false

#if TRY_SET_HUD_OPTIONS()
#include "tac-win32/tac_win32.h"
#include "WinPixEventRuntime/pix3.h" // include after tac_win32.h
using PixSetHUDOptionsSig = HRESULT(WINAPI* )(PIXHUDOptions);
static PixSetHUDOptionsSig sPixSetHUDOptions;
#endif

namespace Tac::Render
{
  static const FileSys::Path pixInstallPath { "C:/Program Files/Microsoft PIX" };
  static const char*         pixDllName     { "WinPixGpuCapturer.dll" };

  // Looking at https://devblogs.microsoft.com/pix/download/
  // Pix version numbers seem to usually be XXXX.YY 
  static bool IsVersionPattern(StringView sv)
  {
    if( sv.size() != 7 )
      return false;

    const char* p { sv.data() };

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

  static FileSys::Path TryFindPIXDllPath(Errors& errors)
  {
    if( !FileSys::Exists( pixInstallPath ) )
      return {};

    TAC_CALL_RET( const FileSys::Paths subdirs{ FileSys::IterateDirectories(
      pixInstallPath,
      FileSys::IterateType::Default,
      errors ) }  );

    if( subdirs.empty() )
      return {};

    String bestVer;
    for( const FileSys::Path& subdir : subdirs )
    {
      const String ver { subdir.dirname().u8string() };
      if( !IsVersionPattern( ver ) || ver < bestVer )// ( !bestSubdir.empty() && ver < bestSubdir ) )
        continue;

      bestVer = ver;
    }

    if( !bestVer.empty() )
    {
      const String minVer { "2312.08"  };
      TAC_RAISE_ERROR_IF_RETURN( {},
                                   bestVer < minVer,
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
      if( !OS::CmdLineIsFlagPresent( "allowpixattach" ) )
        return;

      // Check to see if a copy of WinPixGpuCapturer.dll has already been injected into the application.
      // This may happen if the application is launched through the PIX UI. 
      if( OS::OSGetLoadedDLL( pixDllName ) )
        return;

      const FileSys::Path path{ TryFindPIXDllPath( errors ) };
      const String path8{ path.u8string() };
      if( path8.empty() )
      {
        OS::OSDebugPrintLine( String() + "Warning: Could not find PIX dll " + pixDllName
                              + ". Is it installed? "
                              "PIX will not be able to attach to the running process." );
        return;
      }

      void* lib{ OS::OSLoadDLL( path.u8string() ) };
      if( !lib )
      {
        OS::OSDebugPrintLine( String() +
                              "Failed to load PIX dll " + pixDllName + " at path " + path8 + "." );
      }


#if TRY_SET_HUD_OPTIONS()
        sPixSetHUDOptions = (PixSetHUDOptionsSig)OS::OSGetProcAddress( lib, "PixSetHUDOptions" );
        if( sPixSetHUDOptions )
          sPixSetHUDOptions( PIX_HUD_SHOW_ON_NO_WINDOWS );
#endif
    }
  }

} // namespace Tac::Render



