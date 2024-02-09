#include "src/shell/tac_desktop_event.h" // self-include

#include "src/common/containers/tac_ring_buffer.h"
#include "src/common/preprocess/tac_preprocessor.h"
#include "src/shell/tac_desktop_window_graphics.h"
#include "src/shell/tac_desktop_app.h"

import std; // mutex, lock_guard, is_trivially_copyable_v

namespace Tac
{

  enum class DesktopEventType
  {
    WindowAssignHandle,
    WindowResize,
    WindowMove,
    CursorUnobscured,
    KeyState,
    MouseButtonState,
    KeyInput,
    MouseWheel,
    MouseMove,
    Count,
  };

  // -----------------------------------------------------------------------------------------------

  struct DesktopEventQueueImpl
  {
    void       Init();

    template< typename T >
    void       QueuePush( const DesktopEventType type, const T* t )
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

  using EventHandler = void( * )( );

  static DesktopEventQueueImpl         sEventQueue;
  static EventHandler sEventHandlers[ ( int )DesktopEventType::Count ];

  // -----------------------------------------------------------------------------------------------


  static void HandleDesktopEvent( DesktopEventDataAssignHandle data )
  {

    DesktopWindowState* desktopWindowState = GetDesktopWindowState( data.mDesktopWindowHandle );
    if( desktopWindowState->mNativeWindowHandle != data.mNativeWindowHandle )
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
    desktopWindowState->mName = ( StringView )data.mName;
    desktopWindowState->mWidth = data.mW;
    desktopWindowState->mHeight = data.mH;
    desktopWindowState->mX = data.mX;
    desktopWindowState->mY = data.mY;
  }

  static void HandleDesktopEvent( DesktopEventDataWindowMove data )
  {
    DesktopWindowState* state = GetDesktopWindowState( data.mDesktopWindowHandle );
    state->mX = data.mX;
    state->mY = data.mY;
  }

  static void HandleDesktopEvent( DesktopEventDataWindowResize data )
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( data.mDesktopWindowHandle );
    desktopWindowState->mWidth = data.mWidth;
    desktopWindowState->mHeight = data.mHeight;
    WindowGraphicsResize( data.mDesktopWindowHandle,
                          desktopWindowState->mWidth,
                          desktopWindowState->mHeight );
  }

  static void HandleDesktopEvent( DesktopEventDataKeyInput data )
  {
    Keyboard::KeyboardSetWMCharPressedHax( data.mCodepoint );
  }

  static void HandleDesktopEvent( DesktopEventDataKeyState data )
  {
    Keyboard::KeyboardSetIsKeyDown( data.mKey, data.mDown );
  }

  static void HandleDesktopEvent( DesktopEventDataMouseButtonState data )
  {
    Mouse::ButtonSetIsDown( data.mButton, data.mDown );
  }

  static void HandleDesktopEvent( DesktopEventDataMouseWheel data )
  {
    Mouse::MouseWheelEvent( data.mDelta );
  }

  static void HandleDesktopEvent( DesktopEventDataMouseMove data )
  {
    const DesktopWindowState* desktopWindowState = GetDesktopWindowState( data.mDesktopWindowHandle );
    const v2 windowPos = desktopWindowState->GetPosV2();
    const v2 dataPos( ( float )data.mX, ( float )data.mY );
    const v2 pos = windowPos + dataPos;
    Mouse::SetScreenspaceCursorPos( pos );
  }

  static void HandleDesktopEvent( DesktopEventDataCursorUnobscured data )
  {
    SetHoveredWindow( data.mDesktopWindowHandle );
  }

  template< typename T >
  static void HandleDesktopEventT()
  {
      const auto data = sEventQueue.QueuePop<T>();
      HandleDesktopEvent( data );
  }

  // -----------------------------------------------------------------------------------------------

  static void RegisterDesktopEventHandler( DesktopEventType type, EventHandler handler )
  {
    sEventHandlers[ ( int )type ] = handler;
  }

  template< typename T >
  static void RegisterDesktopEventHandlerT( DesktopEventType type )
  {
     EventHandler handler  = HandleDesktopEventT< T >;

    RegisterDesktopEventHandler( type, handler );
  }

  void         DesktopEventInit()
  {
    TAC_ASSERT( IsMainThread() );
    sEventQueue.Init();

    RegisterDesktopEventHandlerT<DesktopEventDataAssignHandle>( DesktopEventType::WindowAssignHandle );
    RegisterDesktopEventHandlerT<DesktopEventDataWindowResize>( DesktopEventType::WindowResize );
    RegisterDesktopEventHandlerT<DesktopEventDataWindowMove>( DesktopEventType::WindowMove );
    RegisterDesktopEventHandlerT<DesktopEventDataCursorUnobscured>( DesktopEventType::CursorUnobscured );
    RegisterDesktopEventHandlerT<DesktopEventDataKeyState>( DesktopEventType::KeyState);
    RegisterDesktopEventHandlerT<DesktopEventDataMouseButtonState>( DesktopEventType::MouseButtonState);
    RegisterDesktopEventHandlerT<DesktopEventDataKeyInput>( DesktopEventType::KeyInput);
    RegisterDesktopEventHandlerT<DesktopEventDataMouseWheel>( DesktopEventType::MouseWheel);
    RegisterDesktopEventHandlerT<DesktopEventDataMouseMove>( DesktopEventType::MouseMove);
  }

  void         DesktopEventApplyQueue()
  {
    TAC_ASSERT( IsLogicThread() );
    while( !sEventQueue.Empty() )
    {
      const auto desktopEventType = sEventQueue.QueuePop<DesktopEventType>();
      const EventHandler handler = sEventHandlers[ (int)desktopEventType ];
      TAC_ASSERT(handler);
      handler();
    }
  }


  // -----------------------------------------------------------------------------------------------

  void                DesktopEvent( const DesktopEventDataAssignHandle& data )
  {
    sEventQueue.QueuePush( DesktopEventType::WindowAssignHandle, &data );
  }

  void                DesktopEvent( const DesktopEventDataWindowMove& data )
  {
    sEventQueue.QueuePush( DesktopEventType::WindowMove, &data );
  }

  void                DesktopEvent( const DesktopEventDataWindowResize& data )
  {
    sEventQueue.QueuePush( DesktopEventType::WindowResize, &data );
  }

  void                DesktopEvent( const DesktopEventDataMouseWheel& data )
  {
    sEventQueue.QueuePush( DesktopEventType::MouseWheel, &data );
  }

  void                DesktopEvent( const DesktopEventDataMouseMove& data )
  {
    sEventQueue.QueuePush( DesktopEventType::MouseMove, &data );
  }

  void                DesktopEvent( const DesktopEventDataKeyState& data )
  {
    sEventQueue.QueuePush( DesktopEventType::KeyState, &data );
  }

  void                DesktopEvent( const DesktopEventDataMouseButtonState& data )
  {
    sEventQueue.QueuePush( DesktopEventType::MouseButtonState, &data );
  }

  void                DesktopEvent( const DesktopEventDataKeyInput& data )
  {
    sEventQueue.QueuePush( DesktopEventType::KeyInput, &data );
  }

  void                DesktopEvent( const DesktopEventDataCursorUnobscured& data )
  {
    sEventQueue.QueuePush( DesktopEventType::CursorUnobscured, &data );
  }


  // -----------------------------------------------------------------------------------------------

} // namespace Tac
