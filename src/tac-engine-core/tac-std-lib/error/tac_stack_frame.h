#pragma once

#include "tac-std-lib/string/tac_string.h"

namespace Tac
{
  // similar to c++20 std::source_location
  struct StackFrame
  {
    String Stringify() const;
    operator bool() const { return mFile; }
    int         mLine {};
    const char* mFile {};
    const char* mFn   {};
  };
}

#define TAC_STACK_FRAME Tac::StackFrame{ .mLine{ __LINE__ }, .mFile{ __FILE__ }, .mFn{ __func__ } }
