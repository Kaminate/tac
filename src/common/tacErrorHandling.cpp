#include "tacErrorHandling.h"

void TacErrors::operator=( const TacString& message )
{
  mMessage = message;
}
void TacErrors::operator+=( const TacString& message )
{
  mMessage += message;
}
TacString TacErrors::ToString()
{
  TacString result;
  result += mMessage + "\n";
  for(TacStackFrame& frame : mStackFrames)
    result += frame.ToString() + "\n";
  return result;
}
bool TacErrors::size()
{
  return !mMessage.empty();
}
bool TacErrors::empty()
{
  return mMessage.empty();
}
void TacErrors::clear()
{
  mMessage.clear();
  mStackFrames.clear();
}
//void TacErrors::Push( TacString message )
//{
//  mMessage += message;
//}
//void TacErrors::Push( TacString message, TacStackFrame stackFrame )
//{
//  Push( message );
//  Push( stackFrame );
//}
//void TacErrors::Push( TacStackFrame stackFrame )
//{
//  TacAssert( size() );
//}
void TacErrors::Append( const TacStackFrame& stackFrame )
{
  TacAssert( size() );
  mStackFrames.push_back( stackFrame );
}
void TacErrors::Append( const TacString& message )
{
  TacAssert( size() );
  mMessage += message;
}
