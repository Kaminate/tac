#include "src/shell/windows/tac_win32.h"


namespace Tac
{
  struct Errors;

  struct Win32Event
  {
    void Init( Errors& );
    ~Win32Event();
    void clear();

    void operator = ( Win32Event&& other );
    operator bool() const;

    explicit operator HANDLE() const;

    HANDLE mEvent{};
  };
}
