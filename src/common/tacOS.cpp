#include "src/common/tacOS.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacErrorHandling.h"
namespace Tac
{
  namespace OS
  {

    bool mShouldStopRunning = false;

    void CreateFolderIfNotExist( StringView path, Errors& errors )
    {
      bool exist;
      DoesFolderExist( path, exist, errors );
      TAC_HANDLE_ERROR( errors );
      if( exist )
        return;
      CreateFolder( path, errors );
      TAC_HANDLE_ERROR( errors );
    }

    void OS::DebugAssert( StringView msg, const StackFrame& frame )
    {
      String s = msg + "\n" + frame.ToString();
      if( !IsDebugMode() )
        return;
      std::cout << s << std::endl;
      DebugBreak();
      DebugPopupBox( s );
      exit( -1 );
    }
  }
}
