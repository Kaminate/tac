#include "src/common/tacErrorHandling.h"
#include "src/common/tacUtility.h"
#include "src/common/tacOS.h"

namespace Tac
{
  Errors::Errors( bool breakOnAppend )
  {
    mBreakOnAppend = breakOnAppend;
  }
  void Errors::operator=( const char* message )
  {
    mMessage = message;
  }
  void Errors::operator=( StringView message )
  {
    mMessage = message;
  }
  void Errors::operator+=( StringView message )
  {
    mMessage += message;
  }
  String Errors::ToString() const
  {
    Vector< String > strings = { mMessage };
    for( const StackFrame& frame : mFrames )
      strings.push_back( frame.ToString() );
    return Join( strings, "\n" );
  }
  bool Errors::size() const
  {
    return !mMessage.empty();
  }
  bool Errors::empty() const
  {
    return mMessage.empty();
  }
  void Errors::clear()
  {
    mMessage.clear();
    mFrames.clear();
  }
  //void Errors::Push( String message )
  //{
  //  mMessage += message;
  //}
  //void Errors::Push( String message, Frame frame )
  //{
  //  Push( message );
  //  Push( frame );
  //}
  //void Errors::Push( Frame frame )
  //{
  //  Assert( size() );
  //}
  void Errors::Append( const StackFrame& frame )
  {
    TAC_ASSERT( size() );
    if( mBreakOnAppend && mFrames.empty() && IsDebugMode() )
      OS::DebugBreak();
    mFrames.push_back( frame );
  }
  void Errors::Append( StringView message )
  {
    TAC_ASSERT( size() );
    mMessage += message;
  }

  Errors::operator bool() const
  {
    return !mMessage.empty();
  }

}
