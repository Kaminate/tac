#pragma once

namespace Tac
{
  // similar to c++20 std::source_location
  struct StackFrame
  {
    StackFrame() = default;

    // Constructor shortens the __FILE__ macro
    StackFrame( int, const char*, const char* );

    //const char* ToString() const;
    int         mLine     {};
    const char* mFile     {};
    const char* mFunction {};
  };
}

#define TAC_STACK_FRAME Tac::StackFrame( __LINE__, __FILE__, __func__ )

