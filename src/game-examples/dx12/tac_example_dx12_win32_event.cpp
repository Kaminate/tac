#include "tac_example_dx12_win32_event.h" // self-inc

#include "src/common/error/tac_error_handling.h"
#include "src/shell/windows/tac_win32.h" // CreateEvent

namespace Tac
{
  void Win32Event::Init( Errors& errors )
  {
    TAC_ASSERT( !mEvent );

    // Create an event handle to use for frame synchronization.
    mEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );
    TAC_RAISE_ERROR_IF( !mEvent, Win32GetLastErrorString() );
  }

  Win32Event::operator bool() const { return mEvent; }
  Win32Event::operator HANDLE() const { return mEvent; }

  void Win32Event::clear()
  {
    if( mEvent )
    {
      CloseHandle( mEvent );
      mEvent = nullptr;
    }
  }

  Win32Event::~Win32Event()
  {
    clear();
  }

  void Win32Event::operator = ( Win32Event&& other )
  {
    clear();
    mEvent = other.mEvent;
    other.mEvent = nullptr;
  }
} // namespace Tac
