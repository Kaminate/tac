#include "tac_os.h" // self-inc

#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/dataprocess/tac_log.h"

import std; // <cstdio> ( printf ), <thread>, <iostream>

namespace Tac::OS
{

    static bool mStopRunRequested = false;

    decltype( OSDebugBreak )              OSDebugBreak = nullptr;
    decltype( OSDebugPopupBox )           OSDebugPopupBox = nullptr;
    decltype( OSGetApplicationDataPath )  OSGetApplicationDataPath = nullptr;
    decltype( OSSaveDialog )              OSSaveDialog = nullptr;
    decltype( OSOpenDialog )              OSOpenDialog = nullptr;
    decltype( OSGetPrimaryMonitor )       OSGetPrimaryMonitor = nullptr;
    decltype( OSSetScreenspaceCursorPos ) OSSetScreenspaceCursorPos = nullptr;
    decltype( OSGetLoadedDLL )            OSGetLoadedDLL = nullptr;
    decltype( OSLoadDLL )                 OSLoadDLL = nullptr;
    //decltype( OSSemaphoreCreate )         OSSemaphoreCreate = nullptr;
    //decltype( OSSemaphoreDecrementWait )  OSSemaphoreDecrementWait = nullptr;
    //decltype( OSSemaphoreIncrementPost )  OSSemaphoreIncrementPost = nullptr;


    bool        OSAppIsRunning() { return !mStopRunRequested; }

    void        OSAppStopRunning() { mStopRunRequested = true; }

    void        OSDebugAssert( const StringView& msg, const StackFrame& frame )
    {
      const String str = String()
        + "ASSERT FAILED( " + msg + " ) "
        + "in " + frame.mFile + ":" + ToString( frame.mLine ) + " " + frame.mFunction;

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

      const char* szstr = s.c_str();

      //static bool printing; // prevent recursion?
      //if( printing )
      //{
      //  OSDebugBreak();
      //  return;
      //}

      //printing = true;
      std::cout << szstr << std::endl;
      //printing = false;

      const bool good = std::cout.good();
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
