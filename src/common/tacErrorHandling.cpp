#include "src/common/tacErrorHandling.h"

namespace Tac
{

void Errors::operator=( const StringView& message )
{
  mMessage = message;
}
void Errors::operator+=( const StringView& message )
{
  mMessage += message;
}
String Errors::ToString() const
{
  String result;
  result += mMessage + "\n";
  for(const StackFrame& frame : mFrames)
    result += frame.ToString() + "\n";
  return result;
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
  mFrames.push_back( frame );
}
void Errors::Append( const StringView& message )
{
  TAC_ASSERT( size() );
  mMessage += message;
}

Errors::operator bool() const
{
  return !mMessage.empty();
}

}
