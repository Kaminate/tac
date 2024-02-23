#include "src/shell/windows/renderer/pix/tac_pix.h" // self-inc

#include "src/common/preprocess/tac_preprocessor.h"
#include "src/common/system/tac_filesystem.h"
#include "src/common/memory/tac_frame_memory.h"
#include "src/common/system/tac_os.h"
#include "src/common/error/tac_error_handling.h"

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

  static const Filesystem::Path pixInstallPath = "C:/Program Files/Microsoft PIX";
  static const char* pixDllName = "WinPixGpuCapturer.dll";



  // Looking at https://devblogs.microsoft.com/pix/download/
  // Pix version numbers seem to usually be XXXX.YY 
  static bool IsVersionPattern(StringView sv)
  {
    if( sv.size() != 7 )
      return false;

    const char* p = sv.data();

    for( int i = 0; i < 4; ++i )
      if( !IsDigit( *p++ ) )
        return false;

    if( '.' != *p++ )
      return false;

    for( int i = 0; i < 2; ++i )
      if( !IsDigit( *p++ ) )
        return false;

    return true;
  }

  static Filesystem::Path TryFindPIXDllPath(Errors& errors)
  {
    if( !Filesystem::Exists( pixInstallPath ) )
      return {};

    const Filesystem::Paths subdirs = TAC_CALL_RET( {}, Filesystem::IterateDirectories(
      pixInstallPath,
      Filesystem::IterateType::Default,
      errors ) );

    if( subdirs.empty() )
      return {};

    String bestVer;
    for( const Filesystem::Path& subdir : subdirs )
    {
      const String ver = subdir.dirname().u8string();
      if( !IsVersionPattern( ver ) || ver < bestVer )// ( !bestSubdir.empty() && ver < bestSubdir ) )
        continue;

      bestVer = ver;
    }

    if( !bestVer.empty() )
    {
      const String minVer = "2312.08" ;
      TAC_RAISE_ERROR_IF_RETURN( bestVer < minVer,
                                 "Most recent pix version " + bestVer + " "
                                 "is below min required PIX ver " + minVer + " "
                                 "please update PIX", {} );
      return pixInstallPath / bestVer / pixDllName;
    }
    
    return subdirs.front() / pixDllName;
  }

  void AllowPIXDebuggerAttachment( Errors& errors )
  {
    if constexpr( !IsDebugMode )
      return;

    // Check to see if a copy of WinPixGpuCapturer.dll has already been injected into the application.
    // This may happen if the application is launched through the PIX UI. 
    if( OS::OSGetLoadedDLL( pixDllName ) )
      return;

    const Filesystem::Path path = TryFindPIXDllPath( errors );
    const String path8 = path.u8string();
    if( path8.empty() )
    {
      OS::OSDebugPrintLine( String() + "Warning: Could not find PIX dll " + pixDllName
                            + ". Is it installed? "
                            "PIX will not be able to attach to the running process." );
      return;
    }

    void* lib = OS::OSLoadDLL( path.u8string() );
    if( !lib )
    {
      OS::OSDebugPrintLine( String() +
                            "Failed to load PIX dll " + pixDllName + " at path " + path8 + "." );
    }

  }

} // namespace Tac::Render



