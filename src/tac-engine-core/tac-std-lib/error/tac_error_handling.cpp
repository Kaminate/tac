#include "tac_error_handling.h" // self-inc

#include "tac-std-lib/os/tac_os.h" // OSDebugPrintLine, OSDebugBreak
#include "tac-std-lib/string/tac_string_view.h"

namespace Tac
{
  Errors::Errors( Flags flags ) : mFlags( flags ) { }

  String             Errors::ToString() const
  {
    String result = mMessage;
    for( const StackFrame& frame : mFrames )
    {
      result += "\n"; 
      result += String() + frame.GetFile()
        + ":" + Tac::ToString( frame.GetLine() )
        + " " + frame.GetFunction();
    }

    return result;
  }

  bool               Errors::empty() const { return mFrames.empty(); }

  void               Errors::clear()
  {
    mMessage.clear();
    mFrames.clear();
    mBroken = false;
  }

  Span< const StackFrame > Errors::GetFrames() const { return Span( mFrames.data(), mFrames.size() ); }

  StringView         Errors::GetMessage() const { return mMessage; }

  void               Errors::Raise( StringView msg, StackFrame sf )
  {
    mMessage += msg;
    mFrames.push_back( sf );

    if constexpr( kIsDebugMode )
    {
      if( !mBroken && ( mFlags & Flags::kDebugBreaks ) )
      {
        const String errStr{ ToString() };
        OS::OSDebugPrintLine( String( 100, '-' ) );
        OS::OSDebugPrintLine( String( 47, ' ' ) + "ERROR!" );
        OS::OSDebugPrintLine( String( 100, '-' ) );
        OS::OSDebugPrintLine( errStr );
        OS::OSDebugPrintLine( String( 100, '-' ) );
        OS::OSDebugBreak();
        mBroken = true;
      }
    }
  }

  void               Errors::Propagate( StackFrame sf ) { mFrames.push_back( sf ); }

  Errors::operator   bool() const { return !mMessage.empty(); }

} // namespace Tac
