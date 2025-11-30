#include "tac_desktop_event.h" // self-inc

#include "tac-std-lib/containers/tac_ring_buffer.h"
#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/mutex/tac_mutex.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app_threads.h"

//import std; // mutex, lock_guard, is_trivially_copyable_v

namespace Tac
{
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
      T t;
      QueuePop( &t, sizeof( T ) );
      return t;
    }


  private:
    RingBuffer mQueue;
    //Mutex      mMutex;
  };

  // -----------------------------------------------------------------------------------------------

  void DesktopEventQueueImpl::Init()
  {
    const int kQueueCapacity { 1024 };
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
    //TAC_SCOPE_GUARD( LockGuard, mMutex );
    return mQueue.Pop( dataBytes, dataByteCount );
  }

  // -----------------------------------------------------------------------------------------------

  static DesktopEventQueueImpl     sEventQueue;
  static DesktopEventApi::Handler* sHandler;

  // -----------------------------------------------------------------------------------------------

  void DesktopEventApi::Init( Handler* handler )
  {
    sEventQueue.Init();
    sHandler = handler;
  }

  void DesktopEventApi::Queue( const WindowActivationEvent& data ) { sEventQueue.QueuePush( DesktopEventType::WindowActivation, &data, sizeof( WindowActivationEvent ) ); }
  void DesktopEventApi::Queue( const WindowCreateEvent& data )     { sEventQueue.QueuePush( DesktopEventType::WindowCreate,     &data, sizeof( WindowCreateEvent ) ); }
  void DesktopEventApi::Queue( const WindowDestroyEvent& data )    { sEventQueue.QueuePush( DesktopEventType::WindowDestroy,    &data, sizeof( WindowDestroyEvent ) ); }
  void DesktopEventApi::Queue( const CursorUnobscuredEvent& data ) { sEventQueue.QueuePush( DesktopEventType::CursorUnobscured, &data, sizeof( CursorUnobscuredEvent ) ); }
  void DesktopEventApi::Queue( const KeyInputEvent& data )         { sEventQueue.QueuePush( DesktopEventType::KeyInput,         &data, sizeof( KeyInputEvent ) ); }
  void DesktopEventApi::Queue( const KeyStateEvent& data )         { sEventQueue.QueuePush( DesktopEventType::KeyState,         &data, sizeof( KeyStateEvent ) ); }
  void DesktopEventApi::Queue( const MouseMoveEvent& data )        { sEventQueue.QueuePush( DesktopEventType::MouseMove,        &data, sizeof( MouseMoveEvent ) ); }
  void DesktopEventApi::Queue( const MouseWheelEvent& data )       { sEventQueue.QueuePush( DesktopEventType::MouseWheel,       &data, sizeof( MouseWheelEvent ) ); }
  void DesktopEventApi::Queue( const WindowMoveEvent& data )       { sEventQueue.QueuePush( DesktopEventType::WindowMove,       &data, sizeof( WindowMoveEvent ) ); }
  void DesktopEventApi::Queue( const WindowResizeEvent& data )     { sEventQueue.QueuePush( DesktopEventType::WindowResize,     &data, sizeof( WindowResizeEvent ) ); }
  void DesktopEventApi::Queue( const WindowVisibleEvent& data )    { sEventQueue.QueuePush( DesktopEventType::WindowVisible,    &data, sizeof( WindowVisibleEvent ) ); }
  void DesktopEventApi::Queue( const WindowDpiChangedEvent& data ) { sEventQueue.QueuePush( DesktopEventType::WindowVisible,    &data, sizeof( WindowDpiChangedEvent ) ); }

  void DesktopEventApi::Apply( Errors& errors )
  {
    while( !sEventQueue.Empty() )
    {
      const auto desktopEventType { sEventQueue.QueuePop< DesktopEventType >() };
      switch( desktopEventType )
      {
      case DesktopEventType::CursorUnobscured:   sHandler->Handle( sEventQueue.QueuePop< CursorUnobscuredEvent >() );     break;
      case DesktopEventType::KeyInput:           sHandler->Handle( sEventQueue.QueuePop< KeyInputEvent >() );             break;
      case DesktopEventType::KeyState:           sHandler->Handle( sEventQueue.QueuePop< KeyStateEvent >() );             break;
      case DesktopEventType::MouseMove:          sHandler->Handle( sEventQueue.QueuePop< MouseMoveEvent >() );            break;
      case DesktopEventType::MouseWheel:         sHandler->Handle( sEventQueue.QueuePop< MouseWheelEvent >() );           break;
      case DesktopEventType::WindowCreate:       sHandler->Handle( sEventQueue.QueuePop< WindowCreateEvent >(), errors ); break;
      case DesktopEventType::WindowDestroy:      sHandler->Handle( sEventQueue.QueuePop< WindowDestroyEvent >() );        break;
      case DesktopEventType::WindowMove:         sHandler->Handle( sEventQueue.QueuePop< WindowMoveEvent >() );           break;
      case DesktopEventType::WindowResize:       sHandler->Handle( sEventQueue.QueuePop< WindowResizeEvent >(), errors ); break;
      case DesktopEventType::WindowVisible:      sHandler->Handle( sEventQueue.QueuePop< WindowVisibleEvent >() );        break;
      case DesktopEventType::WindowActivation:   sHandler->Handle( sEventQueue.QueuePop< WindowActivationEvent >() );     break;
      case DesktopEventType::WindowDpiChanged:   sHandler->Handle( sEventQueue.QueuePop< WindowDpiChangedEvent >() );     break;
      default: TAC_ASSERT_INVALID_CASE( desktopEventType ); return;
      }
    }
  }

} // namespace Tac
