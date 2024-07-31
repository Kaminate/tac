#include "tac_os.h" // self-inc

#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/dataprocess/tac_log.h"

import std; // <cstdio> ( printf ), <thread>, <iostream>

namespace Tac::OS
{

    static bool mStopRunRequested {};

#define TAC_INIT_PTR( ptr ) decltype( ptr ) ptr {}

    TAC_INIT_PTR( OSDebugBreak );
    TAC_INIT_PTR( OSDebugPopupBox );
    TAC_INIT_PTR( OSGetApplicationDataPath );
    TAC_INIT_PTR( OSSaveDialog );
    TAC_INIT_PTR( OSOpenDialog );
    TAC_INIT_PTR( OSGetPrimaryMonitor );
    TAC_INIT_PTR( OSSetScreenspaceCursorPos );
    TAC_INIT_PTR( OSGetLoadedDLL );
    TAC_INIT_PTR( OSLoadDLL );
    TAC_INIT_PTR( OSSemaphoreCreate );

    bool        OSAppIsRunning() { return !mStopRunRequested; }

    void        OSAppStopRunning() { mStopRunRequested = true; }

    void        OSDebugAssert( const StringView& msg, const StackFrame& frame )
    {
      const String str{ String()
        + "ASSERT FAILED( " + msg + " ) "
        + "in " + frame.GetFile()
        + ":" + ToString( frame.GetLine() ) + " "
        + frame.GetFunction() };

      OSDebugPrintLine( str );
      LogApi::LogMessagePrintLine( str, LogApi::kError );
      OSDebugPopupBox( str );
      OSDebugBreak();
      std::exit( -1 );
    }

    void        OSDebugPrint( const StringView& s )
    {
      if constexpr( IsDebugMode )
      {
        std::cout << s.c_str();
        LogApi::LogMessagePrint( s );
      }
    }

    void        OSDebugPrintLine( const StringView& s )
    {
      if constexpr( !IsDebugMode )
        return;

      const char* szstr { s.c_str() };
      std::cout << szstr << std::endl;
      LogApi::LogMessagePrintLine( s );
      const bool good { std::cout.good() };
      if( !good )
      {
        // this should never happen hopefully
        OSDebugBreak();
      }
    }

    void        OSThreadSleepSec( float t )
    {
      OSThreadSleepMsec( ( int )( t * 1000 ) );
    }

    void        OSThreadSleepMsec( int t )
    {
      std::this_thread::sleep_for( std::chrono::milliseconds( t ) );
    }



} // namespace Tac::OS
