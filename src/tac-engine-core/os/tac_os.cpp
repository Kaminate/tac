#include "tac_os.h" // self-inc

#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/dataprocess/tac_log.h"
#include "tac-std-lib/containers/tac_set.h"
#include "tac-std-lib/string/tac_string.h"

#if TAC_SHOULD_IMPORT_STD()
  import std; // <cstdio> ( printf ), <thread>, <iostream>
#else
  #include <thread>
  #include <iostream>
#endif

namespace Tac::OS
{

  static bool          mStopRunRequested{};
  static Set< String > sFlags;

#define TAC_INIT_PTR( ptr ) decltype( ptr ) ptr {}

  TAC_INIT_PTR( OSDebugBreak );
  TAC_INIT_PTR( OSDebugPopupBox );
  TAC_INIT_PTR( OSGetApplicationDataPath );
  TAC_INIT_PTR( OSSaveDialog );
  TAC_INIT_PTR( OSOpenDialog );
  TAC_INIT_PTR( OSGetPrimaryMonitor );
  TAC_INIT_PTR( OSGetMonitorAtPoint );
  TAC_INIT_PTR( OSGetMonitorFromNativeWindowHandle );
  TAC_INIT_PTR( OSSetScreenspaceCursorPos );
  TAC_INIT_PTR( OSGetLoadedDLL );
  TAC_INIT_PTR( OSLoadDLL );
  TAC_INIT_PTR( OSGetProcAddress );
  TAC_INIT_PTR( OSOpenPath );

}

namespace Tac
{
  bool OS::OSAppIsRunning() { return !mStopRunRequested; }

  void OS::OSAppStopRunning() { mStopRunRequested = true; }



#if 0
  void OS::OSDebugPrint( const StringView s )
  {
    if constexpr( kIsDebugMode )
    {
      std::cout << s.c_str();
      LogApi::LogMessagePrint( s );
    }
  }

  void LogApi::LogMessagePrintLine( const StringView s )
  {
    if constexpr( kIsDebugMode )
    {
      const char* szstr{ s.c_str() };
      std::cout << szstr << std::endl;
      LogApi::LogMessagePrintLine( s );
    }
  }
#endif

  void OS::OSThreadSleepSec( float t )
  {
    OSThreadSleepMsec( ( int )( t * 1000 ) );
  }

  void OS::OSThreadSleepMsec( int t )
  {
    std::this_thread::sleep_for( std::chrono::milliseconds( t ) );
  }

  void OS::CmdLineAddFlag( StringView flag )
  {
    sFlags.insert( flag );
  }

  bool OS::CmdLineIsFlagPresent( StringView flag )
  {
    return sFlags.contains( flag );
  }
} // namespace Tac

