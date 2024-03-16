// The purpose of this file is to wrap lpfnWndProc (Tac::WindowProc).
//
// Event flow:
//   1) A Win32 events from Tac::WindowProc() is queued using Tac::DesktopEvent()
//   2) All queued events are dequeued by Tac::DesktopEventApplyQueue().
//   3) These events are finally handled by Tac::IDesktopEventHandler.
//
#pragma once

#include "tac-engine-core/system/tac_desktop_window.h"
#include "tac-engine-core/i18n/tac_localization.h"
#include "tac-engine-core/input/tac_keyboard_input.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"

namespace Tac::DesktopEventApi
{
  struct AssignHandleEvent
  {
    DesktopWindowHandle mDesktopWindowHandle;
    const void* mNativeWindowHandle = nullptr;
    ShortFixedString    mName;
    int                 mX = 0;
    int                 mY = 0;
    int                 mW = 0;
    int                 mH = 0;
  };

  struct CursorUnobscuredEvent
  {
    DesktopWindowHandle mDesktopWindowHandle;
  };

  struct WindowResizeEvent
  {
    DesktopWindowHandle mDesktopWindowHandle;
    int                 mWidth = 0;
    int                 mHeight = 0;
  };

  struct KeyStateEvent
  {
    Keyboard::Key       mKey = Keyboard::Key::Count;
    bool                mDown = false;
  };

  struct MouseButtonStateEvent
  {
    Mouse::Button       mButton = Mouse::Button::Count;
    bool                mDown = false;
  };

  struct KeyInputEvent
  {
    Codepoint mCodepoint = 0;
  };

  struct MouseWheelEvent
  {
    int mDelta = 0;
  };

  struct MouseMoveEvent
  {
    DesktopWindowHandle mDesktopWindowHandle;
    // Position of the mouse relative to the top left corner of the window
    int                 mX = 0;
    int                 mY = 0;
  };

  struct WindowMoveEvent
    {
      DesktopWindowHandle mDesktopWindowHandle;
      int                 mX = 0;
      int                 mY = 0;
    };

  // -----------------------------------------------------------------------------------------------

  struct Handler
  {
    virtual void Handle( const AssignHandleEvent& ) {};
    virtual void Handle( const CursorUnobscuredEvent& ) {};
    virtual void Handle( const KeyInputEvent& ) {};
    virtual void Handle( const KeyStateEvent& ) {};
    virtual void Handle( const MouseButtonStateEvent& ) {};
    virtual void Handle( const MouseMoveEvent& ) {};
    virtual void Handle( const MouseWheelEvent& ) {};
    virtual void Handle( const WindowMoveEvent& ) {};
    virtual void Handle( const WindowResizeEvent& ) {};
  };

  void Init( Handler* );
  void Queue( const AssignHandleEvent& );
  void Queue( const CursorUnobscuredEvent& );
  void Queue( const KeyInputEvent& );
  void Queue( const KeyStateEvent& );
  void Queue( const MouseButtonStateEvent& );
  void Queue( const MouseMoveEvent& );
  void Queue( const MouseWheelEvent& );
  void Queue( const WindowMoveEvent& );
  void Queue( const WindowResizeEvent& );
  void Apply();


  // -----------------------------------------------------------------------------------------------

} // namespace Tac
