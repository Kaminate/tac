#include "tac_desktop_event_handler.h" // self-inc

#include "tac-engine-core/window/tac_window_backend.h"

namespace Tac
{

  void DesktopEventHandler::Handle( const DesktopEventApi::WindowVisibleEvent& data ) 
  {
    AppWindowApiBackend::SetWindowIsVisible( data.mWindowHandle, data.mVisible );
  }

  void DesktopEventHandler::Handle( const DesktopEventApi::WindowActivationEvent& data )
  {
    // Release all keys on deactivation
    if( data.mState == 0 )
    {
      for( int i{}; i < ( int )Key::Count; ++i )
      {
        const Key key{ ( Key )i };
        AppKeyboardApiBackend::SetKeyState( key, AppKeyboardApiBackend::KeyState::Up );
      }
    }
  }

  void DesktopEventHandler::Handle( const DesktopEventApi::WindowDestroyEvent& data ) 
  {
    AppWindowApiBackend::SetWindowDestroyed( data.mWindowHandle );
  }

  void DesktopEventHandler::Handle( const DesktopEventApi::WindowCreateEvent& data,
                                    Errors& errors ) 
  {
    const v2i pos{ data.mX, data.mY };
    const v2i size{ data.mW, data.mH };
    AppWindowApiBackend::SetWindowCreated( data.mWindowHandle,
                                     data.mNativeWindowHandle,
                                     data.mName,
                                     pos,
                                     size,
                                     errors );
  }

  void DesktopEventHandler::Handle( const DesktopEventApi::CursorUnobscuredEvent& data ) 
  {
    AppWindowApiBackend::SetWindowHovered( data.mWindowHandle );
  }

  void DesktopEventHandler::Handle( const DesktopEventApi::KeyInputEvent& data ) 
  {
    AppKeyboardApiBackend::SetCodepoint( data.mCodepoint );
  }

  void DesktopEventHandler::Handle( const DesktopEventApi::KeyStateEvent& data ) 
  {
    const AppKeyboardApiBackend::KeyState state{ data.mDown
      ? AppKeyboardApiBackend::KeyState::Down
      : AppKeyboardApiBackend::KeyState::Up };

    AppKeyboardApiBackend::SetKeyState( data.mKey, state );
  }

  void DesktopEventHandler::Handle( const DesktopEventApi::MouseMoveEvent& data )
  {
    const v2 screenSpaceWindowPos{ AppWindowApiBackend::GetWindowPos( data.mWindowHandle ) };
    const v2 windowSpaceMousePos{ ( float )data.mX, ( float )data.mY };
    const v2 screenSpaceMousePos{ screenSpaceWindowPos + windowSpaceMousePos };
    AppKeyboardApiBackend::SetScreenspaceMousePos( screenSpaceMousePos );
  }

  void DesktopEventHandler::Handle( const DesktopEventApi::MouseWheelEvent& data ) 
  {
    AppKeyboardApiBackend::AddMouseWheelDelta( data.mDelta );
  }

  void DesktopEventHandler::Handle( const DesktopEventApi::WindowMoveEvent& data ) 
  {
    AppWindowApiBackend::SetWindowPos( data.mWindowHandle, v2i( data.mX, data.mY ) );
  }

  void DesktopEventHandler::Handle( const DesktopEventApi::WindowResizeEvent& data, Errors& errors ) 
  {
    AppWindowApiBackend::SetWindowSize( data.mWindowHandle, v2i( data.mWidth, data.mHeight ), errors );
  }

} // namespace Tac
