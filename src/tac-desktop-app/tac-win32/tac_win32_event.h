#pragma once

#include "tac-win32/tac_win32.h"

namespace Tac
{
  struct Errors;

  struct Win32Event
  {
    ~Win32Event();

    void     Init( Errors& );
    void     clear();
    void     operator = ( Win32Event&& ) noexcept;
    operator bool() const;
    explicit operator HANDLE() const;

    HANDLE mEvent{};
  };
}
