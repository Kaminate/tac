#include "src/shell/tac_desktop_event.h" // self-include

#include "src/common/containers/tac_ring_buffer.h"
#include "src/common/core/tac_preprocessor.h"
#include "src/shell/tac_desktop_window_graphics.h"
#include "src/shell/tac_desktop_app.h"

import std;

namespace Tac
{

  enum class DesktopEventType
  {
    Unknown = 0,
    WindowAssignHandle,
    WindowResize,
    WindowMove,
    CursorUnobscured,
    KeyState,
    MouseButtonState,
    KeyInput,
    MouseWheel,
    MouseMove,
  };

  // -----------------------------------------------------------------------------------------------

  struct DesktopEventQueueImpl
  {
    void       Init();

    template< typename T >
    void       QueuePush( DesktopEventType type, const T* t )
    {
      static_assert( std::is_trivially_copyable_v<T> );
      QueuePush( type, t, sizeof( T ) );
    }

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
    void       QueuePush( DesktopEventType, const void*, int );
    RingBuffer mQueue;
    std::mutex mMutex;
  };

  // -----------------------------------------------------------------------------------------------



  // -----------------------------------------------------------------------------------------------

  void DesktopEventQueueImpl::Init()
  {
    const int kQueueCapacity = 1024;
    mQueue.Init( kQueueCapacity );
  }

  void DesktopEventQueueImpl::QueuePush( DesktopEventType desktopEventType,
                                         const void* dataBytes,
                                         int dataByteCount )
  {
    TAC_ASSERT( IsMainThread() );
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

  static DesktopEventQueueImpl         sEventQueue;

  // -----------------------------------------------------------------------------------------------

  void         DesktopEventInit()
  {
    TAC_ASSERT( IsMainThread() );
    sEventQueue.Init();
  }

  void         DesktopEventApplyQueue()
  {
    TAC_ASSERT( IsLogicThread() );
    while( !sEventQueue.Empty() )
    {
      //DesktopEventType desktopEventType = {};
      //if( !sEventQueue.QueuePop( &desktopEventType, sizeof( DesktopEventType ) ) )
      //  break;
      const auto desktopEventType = sEventQueue.QueuePop<DesktopEventType>();

      switch( desktopEventType )
      {
      case DesktopEventType::WindowAssignHandle:
      {
        //DesktopEventDataAssignHandle data;
        //sEventQueue.QueuePop( &data, sizeof( data ) );
        const auto data = sEventQueue.QueuePop<DesktopEventDataAssignHandle>();
        DesktopWindowState* desktopWindowState = GetDesktopWindowState( data.mDesktopWindowHandle );
        if( desktopWindowState->mNativeWindowHandle != data.mNativeWindowHandle )
          //if( !desktopWindowState->mNativeWindowHandle )
        {
          WindowGraphicsNativeHandleChanged( data.mDesktopWindowHandle,
                                             data.mNativeWindowHandle,
                                             data.mName,
                                             data.mX,
                                             data.mY,
                                             data.mW,
                                             data.mH );
        }
        desktopWindowState->mNativeWindowHandle = data.mNativeWindowHandle;
        desktopWindowState->mName = (StringView)data.mName;
        desktopWindowState->mWidth = data.mW;
        desktopWindowState->mHeight = data.mH;
        desktopWindowState->mX = data.mX;
        desktopWindowState->mY = data.mY;
      } break;

      case DesktopEventType::WindowMove:
      {
        const auto data= sEventQueue.QueuePop<DesktopEventDataWindowMove>();
        DesktopWindowState* state = GetDesktopWindowState( data.mDesktopWindowHandle );
        state->mX = data.mX;
        state->mY = data.mY;
      } break;

      case DesktopEventType::WindowResize:
      {
        const auto data= sEventQueue.QueuePop<DesktopEventDataWindowResize>();
        DesktopWindowState* desktopWindowState = GetDesktopWindowState( data.mDesktopWindowHandle );
        desktopWindowState->mWidth = data.mWidth;
        desktopWindowState->mHeight = data.mHeight;
        WindowGraphicsResize( data.mDesktopWindowHandle,
                              desktopWindowState->mWidth,
                              desktopWindowState->mHeight );
      } break;

      case DesktopEventType::KeyInput:
      {
        const auto data= sEventQueue.QueuePop<DesktopEventDataKeyInput>();
        Keyboard::KeyboardSetWMCharPressedHax(data.mCodepoint);
      } break;

      case DesktopEventType::KeyState:
      {
        const auto data = sEventQueue.QueuePop<DesktopEventDataKeyState>();
        Keyboard::KeyboardSetIsKeyDown( data.mKey, data.mDown );
      } break;

      case DesktopEventType::MouseButtonState:
      {
        const auto  data = sEventQueue.QueuePop<DesktopEventDataMouseButtonState>();
        Mouse::ButtonSetIsDown( data.mButton, data.mDown );
      } break;

      case DesktopEventType::MouseWheel:
      {
        const auto data = sEventQueue.QueuePop<DesktopEventDataMouseWheel>();
        Mouse::MouseWheelEvent(data.mDelta);
      } break;

      case DesktopEventType::MouseMove:
      {
        const auto data = sEventQueue.QueuePop<DesktopEventDataMouseMove>();
        const DesktopWindowState* desktopWindowState = GetDesktopWindowState( data.mDesktopWindowHandle );
        const v2 windowPos = desktopWindowState->GetPosV2();
        const v2 dataPos( ( float )data.mX, ( float )data.mY );
        const v2 pos = windowPos + dataPos;
        Mouse::SetScreenspaceCursorPos( pos );
      } break;

      case DesktopEventType::CursorUnobscured:
      {
        const auto data = sEventQueue.QueuePop<DesktopEventDataCursorUnobscured>();
        SetHoveredWindow( data.mDesktopWindowHandle );
      } break;

      default:
        TAC_ASSERT_INVALID_CASE( desktopEventType );
        break;
      }
    }
  }


  // -----------------------------------------------------------------------------------------------

  void                DesktopEvent( const DesktopEventDataAssignHandle& data)
  {
    sEventQueue.QueuePush( DesktopEventType::WindowAssignHandle, &data );
  }

  void                DesktopEvent(const DesktopEventDataWindowMove& data)
  {
    sEventQueue.QueuePush( DesktopEventType::WindowMove, &data );
  }

  void                DesktopEvent( const DesktopEventDataWindowResize& data)
  {
    sEventQueue.QueuePush( DesktopEventType::WindowResize, &data );
  }

  void                DesktopEvent(const DesktopEventDataMouseWheel& data)
  {
    sEventQueue.QueuePush( DesktopEventType::MouseWheel, &data );
  }

  void                DesktopEvent( const DesktopEventDataMouseMove& data)
  {
    sEventQueue.QueuePush( DesktopEventType::MouseMove, &data );
  }

  void                DesktopEvent( const DesktopEventDataKeyState& data)
  {
    sEventQueue.QueuePush( DesktopEventType::KeyState, &data );
  }

  void                DesktopEvent( const DesktopEventDataMouseButtonState& data)
  {
    sEventQueue.QueuePush( DesktopEventType::MouseButtonState, &data );
  }

  void                DesktopEvent( const DesktopEventDataKeyInput& data )
  {
    sEventQueue.QueuePush( DesktopEventType::KeyInput, &data );
  }

  void                DesktopEvent( const DesktopEventDataCursorUnobscured& data)
  {
    sEventQueue.QueuePush( DesktopEventType::CursorUnobscured, &data );
  }


  // -----------------------------------------------------------------------------------------------

} // namespace Tac
