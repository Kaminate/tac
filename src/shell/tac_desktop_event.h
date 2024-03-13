#pragma once

#include "tac-std-lib/system/tac_desktop_window.h"
#include "tac-std-lib/i18n/tac_localization.h"
#include "tac-std-lib/input/tac_keyboard_input.h"


namespace Tac
{
  struct DesktopEventDataAssignHandle
  {
    DesktopWindowHandle mDesktopWindowHandle;
    const void*         mNativeWindowHandle = nullptr;
    ShortFixedString    mName;
    int                 mX = 0;
    int                 mY = 0;
    int                 mW = 0;
    int                 mH = 0;
  };

  struct DesktopEventDataCursorUnobscured
  {
    DesktopWindowHandle mDesktopWindowHandle;
  };

  struct DesktopEventDataWindowResize
  {
    DesktopWindowHandle mDesktopWindowHandle;
    int                 mWidth = 0;
    int                 mHeight = 0;
  };

  struct DesktopEventDataKeyState
  {
    Keyboard::Key       mKey = Keyboard::Key::Count;
    bool                mDown = false;
  };

  struct DesktopEventDataMouseButtonState
  {
    Mouse::Button       mButton = Mouse::Button::Count;
    bool                mDown = false;
  };

  struct DesktopEventDataKeyInput
  {
    Codepoint mCodepoint = 0;
  };

  struct DesktopEventDataMouseWheel
  {
    int mDelta = 0;
  };

  struct DesktopEventDataMouseMove
  {
    DesktopWindowHandle mDesktopWindowHandle;
    // Position of the mouse relative to the top left corner of the window
    int                 mX = 0;
    int                 mY = 0;
  };

  struct DesktopEventDataWindowMove
  {
    DesktopWindowHandle mDesktopWindowHandle;
    int                 mX = 0;
    int                 mY = 0;
  };

  // -----------------------------------------------------------------------------------------------

  void DesktopEvent( const DesktopEventDataAssignHandle& );
  void DesktopEvent( const DesktopEventDataWindowMove& );
  void DesktopEvent( const DesktopEventDataWindowResize& );
  void DesktopEvent( const DesktopEventDataKeyState& );
  void DesktopEvent( const DesktopEventDataMouseButtonState& );
  void DesktopEvent( const DesktopEventDataKeyInput& );
  void DesktopEvent( const DesktopEventDataMouseWheel& );
  void DesktopEvent( const DesktopEventDataMouseMove& );
  void DesktopEvent( const DesktopEventDataWindowMove& );
  void DesktopEvent( const DesktopEventDataCursorUnobscured& );

  void DesktopEventInit();
  void DesktopEventApplyQueue();

  // -----------------------------------------------------------------------------------------------

} // namespace Tac
