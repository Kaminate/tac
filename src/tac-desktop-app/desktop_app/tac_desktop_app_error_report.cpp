#include "tac_desktop_app_error_report.h" // self-inc

#include "tac_stack_frame_formatter.h"

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/containers/tac_span.h"
#include "tac-std-lib/dataprocess/tac_log.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/containers/tac_vector.h"
//#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
  // -----------------------------------------------------------------------------------------------

  struct NamedError
  {
    String      FormatErrorString() const;

    const char* mName;
    Errors*     mErrorPointer;
  };

  // -----------------------------------------------------------------------------------------------

  String NamedError::FormatErrorString() const
  {
    String errorStr;
    errorStr += "Errors in ";
    errorStr += mName;
    errorStr += '\n';
    errorStr += mErrorPointer->GetMessage();
    errorStr += '\n';
    errorStr += StackFrameFormatter( mErrorPointer->GetFrames() ).FormatFrames();
    return errorStr;
  }

  // -----------------------------------------------------------------------------------------------

  static Vector< NamedError > sNamedErrors;

  void DesktopAppErrorReport::Add( const char* name, Errors* errors )
  {
    const NamedError namedError
    {
      .mName         { name },
      .mErrorPointer { errors },
    };
    sNamedErrors.push_back( namedError );
  }

  void DesktopAppErrorReport::Report()
  {

    String combinedErrorStr;
    for( const NamedError& namedError : sNamedErrors )
      if( !namedError.mErrorPointer->empty() )
        combinedErrorStr += namedError.FormatErrorString() + '\n';

    if( !combinedErrorStr.empty() )
    {
      LogApi::LogMessage( combinedErrorStr );
      LogApi::LogFlush();
      OS::OSDebugPopupBox( combinedErrorStr );
    }
  }

} // namespace Tac
