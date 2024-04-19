#include "tac_desktop_event_handler.h" // self-inc

namespace Tac
{
  void DesktopEventHandler::HandleBegin() 
  {
    mWindowBackend->ApplyBegin();
    mKeyboardBackend->ApplyBegin();
  }

  void DesktopEventHandler::HandleEnd() 
  {
    mWindowBackend->ApplyEnd();
    mKeyboardBackend->ApplyEnd();
  }

  void DesktopEventHandler::Handle( const DesktopEventApi::WindowVisibleEvent& data ) 
  {
    mWindowBackend->SetWindowIsVisible( data.mWindowHandle, data.mVisible );
  }

  void DesktopEventHandler::Handle( const DesktopEventApi::WindowDestroyEvent& data ) 
  {
    mWindowBackend->SetWindowDestroyed( data.mWindowHandle );
  }

  void DesktopEventHandler::Handle( const DesktopEventApi::WindowCreateEvent& data,
                                    Errors& errors ) 
  {
    const v2i pos{ data.mX, data.mY };
    const v2i size{ data.mW, data.mH };
    mWindowBackend->SetWindowCreated( data.mWindowHandle,
                                     data.mNativeWindowHandle,
                                     data.mName,
                                     pos,
                                     size,
                                     errors );
  }

  void DesktopEventHandler::Handle( const DesktopEventApi::CursorUnobscuredEvent& data ) 
  {
    //SetHoveredWindow( data.mWindowHandle );
  }

  void DesktopEventHandler::Handle( const DesktopEventApi::KeyInputEvent& data ) 
  {
    mKeyboardBackend->SetCodepoint( data.mCodepoint );
  }

  void DesktopEventHandler::Handle( const DesktopEventApi::KeyStateEvent& data ) 
  {
    const KeyboardBackend::KeyState state{ data.mDown
      ? KeyboardBackend::KeyState::Down
      : KeyboardBackend::KeyState::Up };

    mKeyboardBackend->SetKeyState( data.mKey, state );
  }

  void DesktopEventHandler::Handle( const DesktopEventApi::MouseMoveEvent& data ) {
    const v2 screenSpaceWindowPos{ mWindowBackend->GetWindowPos( data.mWindowHandle ) };
    const v2 windowSpaceMousePos{ ( float )data.mX, ( float )data.mY };
    const v2 screenSpaceMousePos{ screenSpaceWindowPos + windowSpaceMousePos };
    mKeyboardBackend->SetMousePos( screenSpaceMousePos );
  }

  void DesktopEventHandler::Handle( const DesktopEventApi::MouseWheelEvent& data ) 
  {
    mKeyboardBackend->SetMouseWheel( data.mDelta );
  }

  void DesktopEventHandler::Handle( const DesktopEventApi::WindowMoveEvent& data ) 
  {
    mWindowBackend->SetWindowPos( data.mWindowHandle, v2i( data.mX, data.mY ) );
  }

  void DesktopEventHandler::Handle( const DesktopEventApi::WindowResizeEvent& data ) 
  {
    mWindowBackend->SetWindowSize( data.mWindowHandle, v2i( data.mWidth, data.mHeight ) );
  }

}
