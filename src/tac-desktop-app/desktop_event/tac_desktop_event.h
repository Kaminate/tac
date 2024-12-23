// The purpose of this file is to wrap lpfnWndProc (Tac::WindowProc).
//
// Event flow:
//   1) A Win32 events from Tac::WindowProc() is queued using Tac::DesktopEvent()
//   2) All queued events are dequeued by Tac::DesktopEventApplyQueue().
//   3) These events are finally handled by Tac::IDesktopEventHandler.
//
#pragma once

#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-engine-core/i18n/tac_localization.h"
#include "tac-engine-core/hid/tac_key.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"

namespace Tac::DesktopEventApi
{

  struct WindowVisibleEvent
  {
    WindowHandle     mWindowHandle {};
    bool             mVisible      {};
  };
  
  struct WindowCreateEvent
  {
    WindowHandle     mWindowHandle       {};
    ShortFixedString mName               {};
    const void*      mNativeWindowHandle {};
    int              mX                  {};
    int              mY                  {};
    int              mW                  {};
    int              mH                  {};
  };

  struct WindowDestroyEvent
  {
    WindowHandle mWindowHandle;
  };

  struct CursorUnobscuredEvent
  {
    WindowHandle mWindowHandle;
  };

  struct WindowResizeEvent
  {
    WindowHandle mWindowHandle {};
    int          mWidth        {};
    int          mHeight       {};
  };

  struct KeyStateEvent
  {
    Key  mKey  { Key::Count };
    bool mDown {};
  };

  struct KeyInputEvent
  {
    Codepoint mCodepoint {};
  };

  struct WindowActivationEvent
  {
    enum State { Deactivated, Activated };
    WindowHandle mWindowHandle;
    State        mState;
  };

  struct MouseWheelEvent
  {
    float mDelta {};
  };

  struct MouseMoveEvent
  {
    // Window that the mouse moved over (?)
    WindowHandle        mWindowHandle {};

    // Position of the mouse relative to the top left corner of the window
    int                 mX            {};
    int                 mY            {};
  };

  struct WindowMoveEvent
  {
    WindowHandle mWindowHandle {};
    int          mX            {};
    int          mY            {};
  };

  // -----------------------------------------------------------------------------------------------

  struct Handler
  {
    virtual void Handle( const CursorUnobscuredEvent& ) {};
    virtual void Handle( const KeyInputEvent& ) {};
    virtual void Handle( const KeyStateEvent& ) {};
    virtual void Handle( const MouseMoveEvent& ) {};
    virtual void Handle( const MouseWheelEvent& ) {};
    virtual void Handle( const WindowCreateEvent&, Errors& ) {};
    virtual void Handle( const WindowDestroyEvent& ) {};
    virtual void Handle( const WindowMoveEvent& ) {};
    virtual void Handle( const WindowResizeEvent&, Errors& ) {};
    virtual void Handle( const WindowVisibleEvent& ) {};
    virtual void Handle( const WindowActivationEvent& ) {};
  };

  void Init( Handler* );
  void Queue( const WindowActivationEvent& );
  void Queue( const WindowCreateEvent& );
  void Queue( const WindowDestroyEvent& );
  void Queue( const CursorUnobscuredEvent& );
  void Queue( const KeyInputEvent& );
  void Queue( const KeyStateEvent& );
  void Queue( const MouseMoveEvent& );
  void Queue( const MouseWheelEvent& );
  void Queue( const WindowMoveEvent& );
  void Queue( const WindowResizeEvent& );
  void Queue( const WindowVisibleEvent& );

  void Apply( Errors& );


  // -----------------------------------------------------------------------------------------------

} // namespace Tac
