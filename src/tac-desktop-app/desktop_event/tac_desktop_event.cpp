#include "tac_desktop_event.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app_threads.h"
#include "tac-desktop-app/desktop_event/tac_desktop_event.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-engine-core/hid/tac_keyboard_backend.h"
#include "tac-engine-core/window/tac_window_backend.h"
#include "tac-std-lib/containers/tac_ring_buffer.h"
#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/mutex/tac_mutex.h"

namespace Tac
{
  static const int kQueueCapacity { 1024 };

  enum class DesktopEventType
  {
    CursorUnobscured,
    KeyInput,
    KeyState,
    MouseMove,
    MouseWheel,
    WindowActivation,
    WindowCreate,
    WindowDestroy,
    WindowMove,
    WindowResize,
    WindowVisible,
    WindowDpiChanged,
    Count,
  };

  struct DesktopEventQueueImpl
  {
    void Init();
    void QueuePush( DesktopEventType, const void*, int );
    bool QueuePop( void*, int );
    bool Empty();

    template< typename T >
    auto QueuePop() -> T
    {
      TAC_ASSERT( !Empty() );
      T t{};
      QueuePop( &t, sizeof( T ) );
      return t;
    }

    RingBuffer mQueue;
  };

  // -----------------------------------------------------------------------------------------------

  void DesktopEventQueueImpl::Init()
  {
    mQueue.Init( kQueueCapacity );
  }

  void DesktopEventQueueImpl::QueuePush( const DesktopEventType desktopEventType,
                                         const void* dataBytes,
                                         const int dataByteCount )
  {
    // Tac::WindowProc still spews out events while a popupbox is open
    if( mQueue.capacity() - mQueue.size() < ( int )sizeof( DesktopEventType ) + dataByteCount )
      return;

    mQueue.Push( &desktopEventType, sizeof( DesktopEventType ) );
    mQueue.Push( dataBytes, dataByteCount );
  }

  bool DesktopEventQueueImpl::Empty()
  {
    //TAC_SCOPE_GUARD( LockGuard, mMutex );
    return mQueue.Empty();
  }

  bool DesktopEventQueueImpl::QueuePop( void* dataBytes, int dataByteCount )
  {
    return mQueue.Pop( dataBytes, dataByteCount );
  }

  // -----------------------------------------------------------------------------------------------

  static DesktopEventQueueImpl     sEventQueue;

  // -----------------------------------------------------------------------------------------------

  void DesktopEventApi::Init()
  {
    sEventQueue.Init();
  }

  void DesktopEventApi::QueueEvent( const WindowActivationEvent& data ) { sEventQueue.QueuePush( DesktopEventType::WindowActivation, &data, sizeof( WindowActivationEvent ) ); }
  void DesktopEventApi::QueueEvent( const WindowCreateEvent& data )     { sEventQueue.QueuePush( DesktopEventType::WindowCreate,     &data, sizeof( WindowCreateEvent ) ); }
  void DesktopEventApi::QueueEvent( const WindowDestroyEvent& data )    { sEventQueue.QueuePush( DesktopEventType::WindowDestroy,    &data, sizeof( WindowDestroyEvent ) ); }
  void DesktopEventApi::QueueEvent( const CursorUnobscuredEvent& data ) { sEventQueue.QueuePush( DesktopEventType::CursorUnobscured, &data, sizeof( CursorUnobscuredEvent ) ); }
  void DesktopEventApi::QueueEvent( const KeyInputEvent& data )         { sEventQueue.QueuePush( DesktopEventType::KeyInput,         &data, sizeof( KeyInputEvent ) ); }
  void DesktopEventApi::QueueEvent( const KeyStateEvent& data )         { sEventQueue.QueuePush( DesktopEventType::KeyState,         &data, sizeof( KeyStateEvent ) ); }
  void DesktopEventApi::QueueEvent( const MouseMoveEvent& data )        { sEventQueue.QueuePush( DesktopEventType::MouseMove,        &data, sizeof( MouseMoveEvent ) ); }
  void DesktopEventApi::QueueEvent( const MouseWheelEvent& data )       { sEventQueue.QueuePush( DesktopEventType::MouseWheel,       &data, sizeof( MouseWheelEvent ) ); }
  void DesktopEventApi::QueueEvent( const WindowMoveEvent& data )       { sEventQueue.QueuePush( DesktopEventType::WindowMove,       &data, sizeof( WindowMoveEvent ) ); }
  void DesktopEventApi::QueueEvent( const WindowResizeEvent& data )     { sEventQueue.QueuePush( DesktopEventType::WindowResize,     &data, sizeof( WindowResizeEvent ) ); }
  void DesktopEventApi::QueueEvent( const WindowVisibleEvent& data )    { sEventQueue.QueuePush( DesktopEventType::WindowVisible,    &data, sizeof( WindowVisibleEvent ) ); }
  void DesktopEventApi::QueueEvent( const WindowDpiChangedEvent& data ) { sEventQueue.QueuePush( DesktopEventType::WindowVisible,    &data, sizeof( WindowDpiChangedEvent ) ); }

  void DesktopEventApi::ApplyQueuedEvents( Errors& errors )
  {
    while( !sEventQueue.Empty() )
    {
      switch( const auto desktopEventType { sEventQueue.QueuePop< DesktopEventType >() } )
      {
      case DesktopEventType::CursorUnobscured:   HandleEvent( sEventQueue.QueuePop< CursorUnobscuredEvent >() );     break;
      case DesktopEventType::KeyInput:           HandleEvent( sEventQueue.QueuePop< KeyInputEvent >() );             break;
      case DesktopEventType::KeyState:           HandleEvent( sEventQueue.QueuePop< KeyStateEvent >() );             break;
      case DesktopEventType::MouseMove:          HandleEvent( sEventQueue.QueuePop< MouseMoveEvent >() );            break;
      case DesktopEventType::MouseWheel:         HandleEvent( sEventQueue.QueuePop< MouseWheelEvent >() );           break;
      case DesktopEventType::WindowCreate:       HandleEvent( sEventQueue.QueuePop< WindowCreateEvent >(), errors ); break;
      case DesktopEventType::WindowDestroy:      HandleEvent( sEventQueue.QueuePop< WindowDestroyEvent >() );        break;
      case DesktopEventType::WindowMove:         HandleEvent( sEventQueue.QueuePop< WindowMoveEvent >() );           break;
      case DesktopEventType::WindowResize:       HandleEvent( sEventQueue.QueuePop< WindowResizeEvent >(), errors ); break;
      case DesktopEventType::WindowVisible:      HandleEvent( sEventQueue.QueuePop< WindowVisibleEvent >() );        break;
      case DesktopEventType::WindowActivation:   HandleEvent( sEventQueue.QueuePop< WindowActivationEvent >() );     break;
      case DesktopEventType::WindowDpiChanged:   HandleEvent( sEventQueue.QueuePop< WindowDpiChangedEvent >() );     break;
      default: TAC_ASSERT_INVALID_CASE( desktopEventType ); return;
      }
    }
  }


  void DesktopEventApi::HandleEvent( const DesktopEventApi::WindowVisibleEvent& data ) 
  {
    if( AppWindowApiBackend::WindowState * windowState{
      AppWindowApiBackend::FindWindowStateFromNWH( data.mWindowHandle ) } )
      windowState->mShown = data.mVisible;
  }

  void DesktopEventApi::HandleEvent( const DesktopEventApi::WindowActivationEvent& data )
  {
    // Release all keys on deactivation
    if( data.mState == DesktopEventApi::WindowActivationEvent::State::Deactivated )
    {
      for( int i{}; i < ( int )Key::Count; ++i )
      {
        const Key key{ ( Key )i };
        AppKeyboardApiBackend::sUIKeyboardBackend.SetKeyState( key, AppKeyboardApiBackend::KeyState::Up );
        AppKeyboardApiBackend::sGameKeyboardBackend.SetKeyState( key, AppKeyboardApiBackend::KeyState::Up );
      }
    }
  }

  void DesktopEventApi::HandleEvent( const DesktopEventApi::WindowDestroyEvent& data ) 
  {
    AppWindowApiBackend::OnWindowDestroyed( data.mWindowHandle );
  }

  void DesktopEventApi::HandleEvent( const DesktopEventApi::WindowCreateEvent& data, Errors& errors ) 
  {
    const AppWindowApiBackend::WindowCreatedParams params
    {
      .mNWH  { data.mNativeWindowHandle },
      .mName { data.mName },
      .mPos  { data.mX, data.mY  },
      .mSize { data.mW, data.mH  },
    };
    AppWindowApiBackend::OnWindowCreated( params, errors );
  }

  void DesktopEventApi::HandleEvent( const DesktopEventApi::CursorUnobscuredEvent& data ) 
  {
    AppWindowApiBackend::sAppHovered = data.mWindowHandle;
  }

  void DesktopEventApi::HandleEvent( const DesktopEventApi::KeyInputEvent& data ) 
  {
    AppKeyboardApiBackend::sUIKeyboardBackend.SetCodepoint( data.mCodepoint );
    if( !UIKeyboardApi::sWantCaptureKeyboard )
      AppKeyboardApiBackend::sGameKeyboardBackend.SetCodepoint( data.mCodepoint );
  }

  void DesktopEventApi::HandleEvent( const DesktopEventApi::KeyStateEvent& data ) 
  {
    const AppKeyboardApiBackend::KeyState state{ data.mDown
      ? AppKeyboardApiBackend::KeyState::Down
      : AppKeyboardApiBackend::KeyState::Up };

    AppKeyboardApiBackend::sUIKeyboardBackend.SetKeyState( data.mKey, state );
    if( !UIKeyboardApi::sWantCaptureKeyboard )
      AppKeyboardApiBackend::sGameKeyboardBackend.SetKeyState( data.mKey, state );
  }

  void DesktopEventApi::HandleEvent( const DesktopEventApi::MouseMoveEvent& data )
  {
    if( const AppWindowApiBackend::WindowState * windowState{
      AppWindowApiBackend::FindWindowStateFromNWH( data.mWindowHandle ) } )
    {
      const v2 screenSpaceMousePos{ windowState->mPos + v2 { ( float )data.mX, ( float )data.mY } };
      AppKeyboardApiBackend::sUIKeyboardBackend.SetScreenspaceMousePos( screenSpaceMousePos );
      if( !UIKeyboardApi::sWantCaptureMouse )
        AppKeyboardApiBackend::sGameKeyboardBackend.SetScreenspaceMousePos( screenSpaceMousePos );
    }

  }

  void DesktopEventApi::HandleEvent( const DesktopEventApi::MouseWheelEvent& data ) 
  {
    AppKeyboardApiBackend::sUIKeyboardBackend.AddMouseWheelDelta( data.mDelta );
    if( !UIKeyboardApi::sWantCaptureMouse )
      AppKeyboardApiBackend::sGameKeyboardBackend.AddMouseWheelDelta( data.mDelta );
  }

  void DesktopEventApi::HandleEvent( const DesktopEventApi::WindowMoveEvent& data ) 
  {
    if( AppWindowApiBackend::WindowState * windowState{
      AppWindowApiBackend::FindWindowStateFromNWH( data.mWindowHandle ) } )
      windowState->mPos = v2i( data.mX, data.mY );
  }

  void DesktopEventApi::HandleEvent( const DesktopEventApi::WindowResizeEvent& data, Errors& errors ) 
  {
    AppWindowApiBackend::OnWindowSize( data.mWindowHandle, v2i( data.mWidth, data.mHeight ), errors );
  }

  void DesktopEventApi::HandleEvent( const DesktopEventApi::WindowDpiChangedEvent& data )
  {
    WindowHandle wh{ AppWindowApiBackend::FindWindowHandleFromNWH( data.mWindowHandle ) };
    ImGuiPlatformHandleDpiChange( wh );
  }


} // namespace Tac
