#include "tac_desktop_app_error_report.h" // self-inc

#include "tac_stack_frame_formatter.h"

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/containers/tac_span.h"
#include "tac-std-lib/dataprocess/tac_log.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
  // -----------------------------------------------------------------------------------------------

  struct NamedErrors
  {
    auto FormatErrorString( int i ) const -> String
    {
      const char* name{ mNames[i]};
      const Errors* errors{ mErrorPointers[i]};
      if( errors->empty() )
        return {};

      auto stackFrames{ errors->GetFrames() };
      String errorStr;
      errorStr += "Errors in ";
      errorStr += name;
      errorStr += '\n';
      errorStr += errors->GetMessage();
      for( const StackFrame& sf : stackFrames )
        errorStr += '\n' + sf.Stringify();
      return errorStr;
    }

    auto FormatErrorString() const -> String
    {
      String result;

      const int n{ mNames.size() };
      for( int i{}; i < n; ++i )
      {
        String formattedError{ FormatErrorString( i ) };
        if( formattedError.empty() )
          continue;
        result += formattedError;
        result += "\n";
      }

      return result;
    }

    void Add( const char* name, Errors* errors )
    {
      mNames.push_back( name );
      mErrorPointers.push_back( errors );
    }

    Vector< const char* > mNames;
    Vector< Errors* >     mErrorPointers;
  };

  // -----------------------------------------------------------------------------------------------

  static NamedErrors sNamedErrors;

  void DesktopAppErrorReport::Add( const char* name, Errors* errors )
  {
    sNamedErrors.Add( name, errors );
  }

  void DesktopAppErrorReport::Report( Errors* errors )
  {
    Add( "errors", errors );
    Report();
  }

  void DesktopAppErrorReport::Report()
  {
    const String combinedErrorStr{ sNamedErrors.FormatErrorString() };
    if( !combinedErrorStr.empty() )
    {
      LogApi::LogMessagePrintLine( combinedErrorStr, LogApi::kError );
      OS::OSDebugPopupBox( combinedErrorStr );
    }
  }

} // namespace Tac
