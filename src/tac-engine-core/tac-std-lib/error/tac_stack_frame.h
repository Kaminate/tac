#pragma once

namespace Tac
{
  // similar to c++20 std::source_location
  struct StackFrame
  {
    StackFrame() = default;
    StackFrame( int, const char*, const char* );

    bool        IsValid() const;

    int         GetLine() const;
    const char* GetFile() const;
    const char* GetFunction() const;
    
  private:
    int         mLine     {};
    const char* mFile     {};
    const char* mFunction {};
  };
}

#define TAC_STACK_FRAME Tac::StackFrame( __LINE__, __FILE__, __func__ )

