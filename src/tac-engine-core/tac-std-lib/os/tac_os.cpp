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

  static bool mStopRunRequested{};

#define TAC_INIT_PTR( ptr ) decltype( ptr ) ptr {}

  TAC_INIT_PTR( OSDebugBreak );
  TAC_INIT_PTR( OSDebugPopupBox );
  TAC_INIT_PTR( OSGetApplicationDataPath );
  TAC_INIT_PTR( OSSaveDialog );
  TAC_INIT_PTR( OSOpenDialog );
  TAC_INIT_PTR( OSGetPrimaryMonitor );
  TAC_INIT_PTR( OSGetMonitorAtPoint );
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

  void OS::OSDebugAssert( const StringView& msg, const StackFrame& frame )
  {
    const String str{ String()
      + "ASSERT FAILED( " + msg + " ) "
      + "in " + frame.Stringify() };
    OSDebugPrintLine( str );
    LogApi::LogMessagePrintLine( str, LogApi::kError );
    OSDebugPopupBox( str );
    OSDebugBreak();
    if constexpr( !kIsDebugMode )
    {
      std::exit( -1 );
    }
    // TODO: c++26 <debugging> std::is_debugger_present
  }

  void OS::OSDebugPrint( const StringView& s )
  {
    if constexpr( kIsDebugMode )
    {
      std::cout << s.c_str();
      LogApi::LogMessagePrint( s );
    }
  }

  void OS::OSDebugPrintLine( const StringView& s )
  {
    if constexpr( kIsDebugMode )
    {
      const char* szstr{ s.c_str() };
      std::cout << szstr << std::endl;
      LogApi::LogMessagePrintLine( s );
      const bool good{ std::cout.good() };
      if( !good )
      {
        // this should never happen hopefully
        OSDebugBreak();
      }
    }
  }

  void OS::OSThreadSleepSec( float t )
  {
    OSThreadSleepMsec( ( int )( t * 1000 ) );
  }

  void OS::OSThreadSleepMsec( int t )
  {
    std::this_thread::sleep_for( std::chrono::milliseconds( t ) );
  }


  static Set< String > sFlags;
  void Tac::OS::CmdLineAddFlag( StringView flag )
  {
    sFlags.insert( flag );
  }

  bool Tac::OS::CmdLineIsFlagPresent( StringView flag )
  {
    return sFlags.contains( flag );
  }
} // namespace Tac

