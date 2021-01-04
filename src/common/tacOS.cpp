#include "src/common/tacOS.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacErrorHandling.h"

#include <thread>

namespace Tac
{
  namespace OS
  {
    static bool mStopRunRequested = false;

    bool        IsRunning() { return !mStopRunRequested; }
    void        StopRunning() { mStopRunRequested = true; }

    void        CreateFolderIfNotExist( StringView path, Errors& errors )
    {
      bool exist;
      DoesFolderExist( path, exist, errors );
      TAC_HANDLE_ERROR( errors );
      if( exist )
        return;
      CreateFolder( path, errors );
      TAC_HANDLE_ERROR( errors );
    }

    void        DebugAssert( const Errors& errors )
    {
      if( !IsDebugMode() )
        return;
      std::cout << errors.ToString() << std::endl;
      DebugBreak();
      DebugPopupBox( errors.ToString() );
      exit( -1 );
    }

    void        ThreadSleepSec( float t )
    {
      ThreadSleepMsec( ( int )( t * 1000 ) );
    }

    void        ThreadSleepMsec( int t )
    {
      std::this_thread::sleep_for( std::chrono::milliseconds( t ) );
    }
  }
}
