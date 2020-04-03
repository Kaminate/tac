#include "tacErrorHandling.h"

void TacErrors::operator=( const TacStringView& message )
{
  mMessage = message;
}
void TacErrors::operator+=( const TacStringView& message )
{
  mMessage += message;
}
TacString TacErrors::ToString() const
{
  TacString result;
  result += mMessage + "\n";
  for(const TacStackFrame& frame : mStackFrames)
    result += frame.ToString() + "\n";
  return result;
}
bool TacErrors::size() const
{
  return !mMessage.empty();
}
bool TacErrors::empty() const
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
void TacErrors::Append( const TacStringView& message )
{
  TacAssert( size() );
  mMessage += message;
}

TacErrors::operator bool() const
{
  return !mMessage.empty();
}

