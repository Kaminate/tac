#include "src/shell/windows/tacwinlib/renderer/tac_renderer_directx.h"
#include "src/common/tac_preprocessor.h"
#include "src/common/tac_os.h"
#include "src/common/tac_error_handling.h"

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

  static String GetLatestWinPixGpuCapturerPath_Cpp17()
  {
    String pixInstallPath = "C:\\Program Files";
    pixInstallPath += "/Microsoft PIX";

    String newest;

    Errors e;
    bool exist = false;
    GetOS()->OSDoesFolderExist( pixInstallPath, exist, e );
    if( !exist )
      return {};

    Vector< String > files;
    GetOS()->OSGetDirectoriesInDirectory( files, pixInstallPath, e );

    for( String s : files )
      if( newest.empty() || newest < s )
        s = newest;

    TAC_ASSERT( !newest.empty());
    return pixInstallPath + "/" + newest + "WinPixGpuCapturer.dll";
  }

  void AllowPIXDebuggerAttachment()
  {
    // TEMP
    if( true )
      return;

    // Check to see if a copy of WinPixGpuCapturer.dll has already been injected into the application.
    // This may happen if the application is launched through the PIX UI. 
    if( GetOS()->OSGetLoadedDLL( "WinPixGpuCapturer.dll" ) )
      return;
    //HMODULE moduleHandle = GetModuleHandleA( "WinPixGpuCapturer.dll" );
    //if( moduleHandle )
    //  return;

    String path = GetLatestWinPixGpuCapturerPath_Cpp17();
    void* lib = GetOS()->OSLoadDLL( path.c_str() );
    //HMODULE lib = LoadLibrary( path.c_str() );
    if( !lib )
      GetOS()->OSDebugPrintLine(
        "Warning: Could not find WinPixGpuCapturer.dll."
        " PIX (is it installed?) will not be attachable." );
  }

} // namespace Tac



