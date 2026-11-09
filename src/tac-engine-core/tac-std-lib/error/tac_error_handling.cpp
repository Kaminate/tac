#include "tac_error_handling.h" // self-inc

//#include "tac-engine-core/os/tac_os.h" // OSDebugPrintLine, OSDebugBreak
#include "tac-std-lib/dataprocess/tac_log.h"
#include "tac-std-lib/string/tac_string_view.h"

#if 0 // c++26, vs2026
#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <debugging>
#endif
#endif

#if _WIN32
#include <debugapi.h> // IsDebuggerPresent
#endif

namespace Tac
{
  Errors::Errors( Flags flags ) : mFlags( flags ) { }

  auto Errors::ToString() const -> String
  {
    String result{ mMessage };
    for( const StackFrame& frame : mFrames )
      result += "\n" + frame.Stringify();
    return result;
  }

  bool Errors::empty() const { return mFrames.empty(); }

  void Errors::clear()
  {
    mMessage.clear();
    mFrames.clear();
    mBroken = false;
  }

  auto Errors::GetFrames() const -> Span< const StackFrame > { return { mFrames.data(), mFrames.size() }; }

  auto Errors::GetMessage() const ->StringView{ return mMessage; }

  void Errors::Raise( StringView msg, StackFrame sf )
  {
    mMessage += msg;
    mFrames.push_back( sf );

    if constexpr( kIsDebugMode )
    {
      if( !mBroken && ( mFlags & Flags::kDebugBreaks ) )
      {
        const String errStr{ ToString() };
        LogApi::LogMessagePrintLine( String( 100, '-' ) );
        LogApi::LogMessagePrintLine( String( 47, ' ' ) + "ERROR!" );
        LogApi::LogMessagePrintLine( String( 100, '-' ) );
        LogApi::LogMessagePrintLine( errStr );
        LogApi::LogMessagePrintLine( String( 100, '-' ) );

#if _WIN32
        if constexpr( kIsDebugMode )
          if( ::IsDebuggerPresent() )
            // If the process is not being debugged, the function uses the search logic of a standard
            // exception handler. In most cases, this causes the calling process to terminate because
            // of an unhandled breakpoint exception.
            ::DebugBreak();
#else
#error todo debug break
#endif

        mBroken = true;
      }
    }
  }

  void Errors::Propagate( StackFrame sf ) { mFrames.push_back( sf ); }

  Errors::operator bool() const { return !mMessage.empty(); }

} // namespace Tac
