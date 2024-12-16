#pragma once

#include "tac-desktop-app/desktop_event/tac_desktop_event.h"
#include "tac-engine-core/hid/tac_keyboard_backend.h"

namespace Tac
{
  struct DesktopEventHandler : public DesktopEventApi::Handler
  {
    void Handle( const DesktopEventApi::CursorUnobscuredEvent& ) override;
    void Handle( const DesktopEventApi::KeyInputEvent& ) override;
    void Handle( const DesktopEventApi::KeyStateEvent& ) override;
    void Handle( const DesktopEventApi::MouseMoveEvent& ) override;
    void Handle( const DesktopEventApi::MouseWheelEvent& ) override;
    void Handle( const DesktopEventApi::WindowCreateEvent&, Errors& ) override;
    void Handle( const DesktopEventApi::WindowDestroyEvent& ) override;
    void Handle( const DesktopEventApi::WindowMoveEvent& ) override;
    void Handle( const DesktopEventApi::WindowResizeEvent&, Errors& ) override;
    void Handle( const DesktopEventApi::WindowVisibleEvent& ) override;
    void Handle( const DesktopEventApi::WindowActivationEvent& ) override;
  };
} // namespace Tac

