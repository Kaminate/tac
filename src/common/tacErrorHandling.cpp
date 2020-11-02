#include "src/common/tacErrorHandling.h"
#include "src/common/tacUtility.h"
#include "src/common/tacOS.h"

namespace Tac
{
  //void Errors::operator=( const char* message )
  //{
  //  mMessage = message;
  //}
  //void Errors::operator=( StringView message )
  //{
  //  mMessage = message;
  //}
  //void Errors::operator+=( StringView message )
  //{
  //  mMessage += message;
  //}
  Errors::Errors( Flags flags ) :
    mFlags( flags ),
    mBroken( false )
  {
  };
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
    mBroken = false;
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
  void Errors::OnAppend()
  {
    if( ( mFlags & Flags::kDebugBreakOnAppend )
        && !mBroken
        && IsDebugMode() )
    {
      OS::DebugBreak();
      mBroken = true;
    }
  }

  void Errors::Append( const StackFrame& frame )
  {
    //TAC_ASSERT( size() ); why is this assert here
    mFrames.push_back( frame );
    OnAppend();
  }

  void Errors::Append( const StringView& message )
  {
    //TAC_ASSERT( size() );
    mMessage += message;
    OnAppend();
  }

  Errors::operator bool() const
  {
    return !mMessage.empty();
  }

}
