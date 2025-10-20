#include "tac_stack_frame_formatter.h" // self-inc

#include "tac-std-lib/math/tac_math.h" // Max

namespace Tac
{
#if 0
  StackFrameFormatter::StackFrameFormatter( Span< const StackFrame > frames )
  {
    for( const StackFrame& frame : frames )
    {
      mMaxLenFilename = Max( mMaxLenFilename, StrLen( frame.GetFile() ) );
      mMaxLenLine = Max( mMaxLenLine, ToString( frame.GetLine() ).size() );
    }
    mFrames = frames;
  }

  String StackFrameFormatter::FmtLen( StringView sv, int n ) const
  {
    return String() + sv + String( n - sv.size(), ' ' );
  }

  String StackFrameFormatter::FmtFile( StackFrame sf ) const
  {
    return FmtLen( sf.GetFile(), mMaxLenFilename );
  }

  String StackFrameFormatter::FmtLine( StackFrame sf ) const
  {
    return FmtLen( Tac::ToString( sf.GetLine() ), mMaxLenLine );
  }

  String StackFrameFormatter::FormatFrames() const
  {
    String result;
    for( const StackFrame& frame : mFrames )
      result += FormatFrame( frame ) + '\n';
    return result;
  }

  String StackFrameFormatter::FormatFrame( StackFrame sf ) const
  {
    return String() +
      "File: " + FmtFile( sf ) + ", "
      "Line: " + FmtLine( sf ) + ", "
      "Fn: " + sf.GetFunction() + "()" "\n";
  }

#endif
} // namespace Tac
