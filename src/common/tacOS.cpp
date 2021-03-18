#include "src/common/tacOS.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacErrorHandling.h"

#include <thread>
#include <iostream>

namespace Tac
{
  static bool mStopRunRequested = false;



  bool        OSAppIsRunning() { return !mStopRunRequested; }

  void        OSAppStopRunning() { mStopRunRequested = true; }

  void        OSCreateFolderIfNotExist( StringView path, Errors& errors )
  {
    bool exist;
    OSDoesFolderExist( path, exist, errors );
    TAC_HANDLE_ERROR( errors );
    if( exist )
      return;
    OSCreateFolder( path, errors );
    TAC_HANDLE_ERROR( errors );
  }

  void        OSDebugAssert( const Errors& errors )
  {
    if( !IsDebugMode() )
      return;
    std::cout << errors.ToString().c_str() << std::endl;
    OSDebugBreak();
    OSDebugPopupBox( errors.ToString() );
    exit( -1 );
  }

  void        OSThreadSleepSec( float t )
  {
    OSThreadSleepMsec( ( int )( t * 1000 ) );
  }

  void        OSThreadSleepMsec( int t )
  {
    std::this_thread::sleep_for( std::chrono::milliseconds( t ) );
  }
}
