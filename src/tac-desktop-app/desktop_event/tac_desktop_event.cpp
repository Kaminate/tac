#include "tac_desktop_event.h" // self-inc

#include "tac-std-lib/containers/tac_ring_buffer.h"
#include "tac-std-lib/error/tac_assert.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app_threads.h"

import std; // mutex, lock_guard, is_trivially_copyable_v

namespace Tac
{
  enum class DesktopEventType
  {
    CursorUnobscured,
    KeyInput,
    KeyState,
    MouseMove,
    MouseWheel,
    WindowCreate,
    WindowDestroy,
    WindowMove,
    WindowResize,
    WindowVisible,
    Count,
  };

  struct DesktopEventQueueImpl
  {
    void       Init();

    void       QueuePush( DesktopEventType, const void*, int );

    bool       QueuePop( void*, int );

    template< typename T >
    T          QueuePop()
    {
      T t;
      QueuePop( &t, sizeof( T ) );
      return t;
    }

    bool       Empty();

  private:
    RingBuffer mQueue;
    std::mutex mMutex;
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
    TAC_ASSERT( DesktopAppThreads::IsSysThread() );
    std::lock_guard< std::mutex > lockGuard( mMutex );
    // Tac::WindowProc still spews out events while a popupbox is open
    if( mQueue.capacity() - mQueue.size() < ( int )sizeof( DesktopEventType ) + dataByteCount )
      return;

    mQueue.Push( &desktopEventType, sizeof( DesktopEventType ) );
    mQueue.Push( dataBytes, dataByteCount );
  }

  bool DesktopEventQueueImpl::Empty()
  {
    std::lock_guard< std::mutex > lockGuard( mMutex );
    return mQueue.Empty();
  }

  bool DesktopEventQueueImpl::QueuePop( void* dataBytes, int dataByteCount )
  {
    std::lock_guard< std::mutex > lockGuard( mMutex );
    return mQueue.Pop( dataBytes, dataByteCount );
  }

  // -----------------------------------------------------------------------------------------------

  static DesktopEventQueueImpl  sEventQueue;


  static DesktopEventApi::Handler* sHandler;

  // -----------------------------------------------------------------------------------------------


  void DesktopEventApi::Init( Handler* handler )
  {
    sEventQueue.Init();
    sHandler = handler;
  }

  void DesktopEventApi::Queue( const WindowCreateEvent& data )     { sEventQueue.QueuePush( DesktopEventType::WindowCreate,       &data, sizeof( WindowCreateEvent ) ); }
  void DesktopEventApi::Queue( const WindowDestroyEvent& data )    { sEventQueue.QueuePush( DesktopEventType::WindowDestroy,      &data, sizeof( WindowDestroyEvent ) ); }
  void DesktopEventApi::Queue( const CursorUnobscuredEvent& data ) { sEventQueue.QueuePush( DesktopEventType::CursorUnobscured,   &data, sizeof( CursorUnobscuredEvent ) ); }
  void DesktopEventApi::Queue( const KeyInputEvent& data )         { sEventQueue.QueuePush( DesktopEventType::KeyInput,           &data, sizeof( KeyInputEvent ) ); }
  void DesktopEventApi::Queue( const KeyStateEvent& data )         { sEventQueue.QueuePush( DesktopEventType::KeyState,           &data, sizeof( KeyStateEvent ) ); }
  void DesktopEventApi::Queue( const MouseMoveEvent& data )        { sEventQueue.QueuePush( DesktopEventType::MouseMove,          &data, sizeof( MouseMoveEvent ) ); }
  void DesktopEventApi::Queue( const MouseWheelEvent& data )       { sEventQueue.QueuePush( DesktopEventType::MouseWheel,         &data, sizeof( MouseWheelEvent ) ); }
  void DesktopEventApi::Queue( const WindowMoveEvent& data )       { sEventQueue.QueuePush( DesktopEventType::WindowMove,         &data, sizeof( WindowMoveEvent ) ); }
  void DesktopEventApi::Queue( const WindowResizeEvent& data )     { sEventQueue.QueuePush( DesktopEventType::WindowResize,       &data, sizeof( WindowResizeEvent ) ); }
  void DesktopEventApi::Queue( const WindowVisibleEvent& data )    { sEventQueue.QueuePush( DesktopEventType::WindowVisible,      &data, sizeof( WindowVisibleEvent ) ); }

  void DesktopEventApi::Apply( Errors& errors )
  {
    TAC_ASSERT( DesktopAppThreads::IsSimThread() );
    sHandler->HandleBegin();
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
      default: TAC_ASSERT_INVALID_CASE( desktopEventType ); return;
      }
    }
    sHandler->HandleEnd();
  }

}
