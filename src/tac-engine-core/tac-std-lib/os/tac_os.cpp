#include "tac_os.h" // self-inc

#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/dataprocess/tac_log.h"

import std; // <cstdio> ( printf ), <thread>, <iostream>

namespace Tac::OS
{

    static bool mStopRunRequested { false };

#define TAC_INIT_PTR( ptr ) decltype( ptr ) ptr { nullptr }

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
        + "in " + frame.mFile + ":" + ToString( frame.mLine ) + " " + frame.mFunction };

      OSDebugPrintLine( str );
      LogApi::LogMessage( str );
      LogApi::LogFlush();
      OSDebugPopupBox( str );
      OSDebugBreak();
      std::exit( -1 );
    }

    void        OSDebugPrintLine( const StringView& s )
    {
      if constexpr( !IsDebugMode )
        return;

      const char* szstr { s.c_str() };

      //static bool printing; // prevent recursion?
      //if( printing )
      //{
      //  OSDebugBreak();
      //  return;
      //}

      //printing = true;
      std::cout << szstr << std::endl;
      //printing = false;

      LogApi::LogMessage( s );

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
