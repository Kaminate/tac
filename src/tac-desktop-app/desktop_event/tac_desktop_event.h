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


namespace Tac
{
  struct DesktopEventApi
  {

    struct WindowVisibleEvent
    {
      NativeWindowHandle mWindowHandle{};
      bool               mVisible{};
    };

    struct WindowCreateEvent
    {
      NativeWindowHandle mWindowHandle{};
      ShortFixedString   mName{};
      const void*        mNativeWindowHandle{};
      int                mX{};
      int                mY{};
      int                mW{};
      int                mH{};
    };

    struct WindowDestroyEvent
    {
      NativeWindowHandle mWindowHandle;
    };

    struct CursorUnobscuredEvent
    {
      WindowHandle mWindowHandle;
    };

    struct WindowResizeEvent
    {
      NativeWindowHandle mWindowHandle{};
      int          mWidth{};
      int          mHeight{};
    };

    struct KeyStateEvent
    {
      Key  mKey{ Key::Count };
      bool mDown{};
    };

    struct KeyInputEvent
    {
      Codepoint mCodepoint{};
    };

    struct WindowActivationEvent
    {
      enum State { Deactivated, Activated };
      NativeWindowHandle mWindowHandle;
      State        mState;
    };

    struct MouseWheelEvent
    {
      float mDelta{};
    };

    struct MouseMoveEvent
    {
      // Window that the mouse moved over (?)
      NativeWindowHandle        mWindowHandle{};

      // Position of the mouse relative to the top left corner of the window
      int                 mX{};
      int                 mY{};
    };

    struct WindowMoveEvent
    {
      NativeWindowHandle mWindowHandle{};
      int          mX{};
      int          mY{};
    };

    struct WindowDpiChangedEvent
    {
      NativeWindowHandle mWindowHandle{};
    };

    // -----------------------------------------------------------------------------------------------


    static void Init();
    static void QueueEvent( const WindowActivationEvent& );
    static void QueueEvent( const WindowCreateEvent& );
    static void QueueEvent( const WindowDestroyEvent& );
    static void QueueEvent( const CursorUnobscuredEvent& );
    static void QueueEvent( const KeyInputEvent& );
    static void QueueEvent( const KeyStateEvent& );
    static void QueueEvent( const MouseMoveEvent& );
    static void QueueEvent( const MouseWheelEvent& );
    static void QueueEvent( const WindowMoveEvent& );
    static void QueueEvent( const WindowResizeEvent& );
    static void QueueEvent( const WindowVisibleEvent& );
    static void QueueEvent( const WindowDpiChangedEvent& );
    static void HandleEvent( const CursorUnobscuredEvent& );
    static void HandleEvent( const KeyInputEvent& );
    static void HandleEvent( const KeyStateEvent& );
    static void HandleEvent( const MouseMoveEvent& );
    static void HandleEvent( const MouseWheelEvent& );
    static void HandleEvent( const WindowCreateEvent&, Errors& );
    static void HandleEvent( const WindowDestroyEvent& );
    static void HandleEvent( const WindowMoveEvent& );
    static void HandleEvent( const WindowResizeEvent&, Errors& );
    static void HandleEvent( const WindowVisibleEvent& );
    static void HandleEvent( const WindowActivationEvent& );
    static void HandleEvent( const WindowDpiChangedEvent& );
    static void ApplyQueuedEvents( Errors& );
  };

} // namespace Tac
