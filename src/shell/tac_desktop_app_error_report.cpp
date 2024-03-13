#include "tac_desktop_app_error_report.h" // self-include

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/containers/tac_span.h"
#include "tac-std-lib/dataprocess/tac_log.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
  // -----------------------------------------------------------------------------------------------
  struct FrameFormatter
  {
    FrameFormatter( const Span< StackFrame > );
    String FormatFrame( StackFrame );

  private:
    String FmtLen( StringView, int );
    String FmtFile( StackFrame );
    String FmtLine( StackFrame );
    int mMaxLenFilename = 0;
    int mMaxLenLine = 0;
  };

  // -----------------------------------------------------------------------------------------------

    FrameFormatter::FrameFormatter( const Span< StackFrame > frames )
    {
      for( const StackFrame& frame : frames )
      {
        mMaxLenFilename = Max( mMaxLenFilename, StrLen( frame.mFile ) );
        mMaxLenLine = Max( mMaxLenLine, ToString( frame.mLine ).size() );
      }
    }

    String FrameFormatter::FmtLen( StringView sv, int n )
    {
      return String() + sv + String( n - sv.size(), ' ' );
    }

    String FrameFormatter::FmtFile( StackFrame sf )
    {
      return FmtLen( sf.mFile, mMaxLenFilename );
    }

    String FrameFormatter::FmtLine( StackFrame sf )
    {
      return FmtLen( Tac::ToString( sf.mLine ), mMaxLenLine );
    }

    String FrameFormatter::FormatFrame( StackFrame sf )
    {
      return String() + 
        "File: " + FmtFile( sf ) + ", "
        "Line: " + FmtLine( sf ) + ", "
        "Fn: " + sf.mFunction + "()" "\n";
    }

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
        const Span< StackFrame > frames = mErrorPointer->GetFrames();

        FrameFormatter frameFormatter( frames );

        String errorStr;
        errorStr += "Errors in ";
        errorStr += mName;
        errorStr += '\n';
        errorStr += mErrorPointer->GetMessage();
        errorStr += '\n';
        for( const StackFrame& frame : frames )
          errorStr += frameFormatter.FormatFrame( frame ) + '\n';

        return errorStr;
      }

  // -----------------------------------------------------------------------------------------------

  static Vector< NamedError > sNamedErrors;

  void DesktopAppErrorReport::Add( const char* name, Errors* errors )
  {
    sNamedErrors.push_back( NamedError{ .mName = name, .mErrorPointer = errors } );
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
