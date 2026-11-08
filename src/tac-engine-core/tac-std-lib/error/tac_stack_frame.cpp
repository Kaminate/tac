#include "tac_stack_frame.h" // self-inc

namespace Tac
{
  auto StackFrame::Stringify() const -> String
  {
    if( !mFile )
      return "n/a";

    const int i{ StringView( mFile ).find_last_of( "/\\" ) };
    const char* filename{ mFile + ( i == StringView::npos ? 0 : i + 1 ) };
    String result{ filename };
    result += ":";
    result += Tac::ToString( mLine );
    result += " ";
    result += mFn;
    return result;
  }
}
