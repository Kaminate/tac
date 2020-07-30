#include "src/shell/tacDesktopEventBackend.h"
#include "src/shell/tacDesktopApp.h"
#include "src/common/tacDesktopWindow.h" // why is this in common
#include "src/common/tacString.h"
#include <mutex>

namespace Tac
{
  DesktopWindowStates gDesktopWindowStates; // non-static

    // oh yeah this is a great file for this shit
  DesktopWindowState* FindDesktopWindowState( DesktopWindowHandle desktopWindowHandle )
  {
    if( !desktopWindowHandle.IsValid() )
      return nullptr;
    for( DesktopWindowState& state : gDesktopWindowStates )
      if( state.mDesktopWindowHandle.mIndex == desktopWindowHandle.mIndex )
        return &state;
    return nullptr;
  }


  namespace DesktopEvent
  {
    const int kQueueCapacity = 1024;

    static std::mutex gMutex;
    static char gQueue[ kQueueCapacity ];
    static int gQueueHead;
    static int gQueueTail;



    struct DataCreateWindow
    {
      DesktopWindowHandle mDesktopWindowHandle;
      int                 mWidth;
      int                 mHeight;
      int                 mX;
      int                 mY;
      void*               mNativeWindowHandle;
    };

    struct DataCursorUnobscured
    {
      DesktopWindowHandle mDesktopWindowHandle;
    };

    struct DataWindowResize
    {
      DesktopWindowHandle mDesktopWindowHandle;
      int                 mWidth;
      int                 mHeight;
    };

    struct DataKeyState
    {
      Key                 mKey;
      bool                mDown;
    };

    struct DataKeyInput
    {
      Codepoint mCodepoint;
    };
    struct DataMouseWheel
    {
      int mDelta;
    };

    struct DataWindowMove
    {
      DesktopWindowHandle mDesktopWindowHandle;
      int                 mX;
      int                 mY;
    };

    enum class Type
    {
      CreateWindow,
      ResizeWindow,
      MoveWindow,
      CursorUnobscured,
      KeyState,
      KeyInput,
      MouseWheel,
    };

    bool                  QueuePop( Type* type );
    bool                  QueuePop( void* dataBytes, int dataByteCount );




    // no this is bad because the mutex. the whole transaction needs to be one tap
    //void* QueuePush( const Type type, const int dataByteCount )
    //{
    //  const int totalByteCount = sizeof( Type ) + dataByteCount;
    //  std::lock_guard< std::mutex > lockGuard( gMutex );
    //  const int gQueuePreTail = gQueueTail + totalByteCount > kQueueCapacity ? 0 : gQueueTail;
    //  MemCpy( gQueue + gQueuePreTail, bytes, totalByteCount );
    //  gQueueTail = gQueuePreTail + totalByteCount;
    //}

    static void QueuePush( void* dataBytes, int dataByteCount )
    {
      int offset = ( gQueueTail + dataByteCount ) > kQueueCapacity ? 0 : gQueueTail;

      MemCpy( gQueue + offset, dataBytes, dataByteCount );
      offset += dataByteCount;

      gQueueTail = offset;
    }

    static void QueuePush( Type type, void* dataBytes, int dataByteCount )
    {
      std::lock_guard< std::mutex > lockGuard( gMutex );
      QueuePush( &type, sizeof( Type ) );
      QueuePush( dataBytes, dataByteCount );
    }

    bool QueuePop( Type* type )
    {
      return QueuePop( type, sizeof( Type ) );
    }

    bool QueuePop( void* dataBytes, int dataByteCount )
    {
      std::lock_guard< std::mutex > lockGuard( gMutex );
      if( gQueueHead == gQueueTail )
        return false;
      const int totalByteCount = dataByteCount;
      //const int gQueuePreHead = ( gQueueHead + totalByteCount ) > kQueueCapacity ? 0 : gQueueHead;
      //MemCpy( bytes, gQueue + gQueuePreHead, byteCount );
      //gQueueHead = gQueuePreHead + byteCount;
      //return true;


      int offset = ( gQueueHead + totalByteCount ) > kQueueCapacity ? 0 : gQueueHead;

      MemCpy( dataBytes, gQueue + offset, dataByteCount );
      offset += dataByteCount;

      gQueueHead = offset;
      return true;

    }


    void PushEventCursorUnobscured( DesktopWindowHandle desktopWindowHandle )
    {
      DataCursorUnobscured data;
      data.mDesktopWindowHandle = desktopWindowHandle;
      QueuePush( Type::CursorUnobscured, &data, sizeof( data ) );
    }

    void PushEventCreateWindow( DesktopWindowHandle desktopWindowHandle,
                                int width,
                                int height,
                                int x,
                                int y,
                                void* nativeWindowHandle )
    {
      DataCreateWindow data;
      data.mDesktopWindowHandle = desktopWindowHandle;
      data.mWidth = width;
      data.mHeight = height;
      data.mNativeWindowHandle = nativeWindowHandle;
      data.mX = x;
      data.mY = y;
      QueuePush( Type::CreateWindow, &data, sizeof( data ) );
    }

    void PushEventMoveWindow( DesktopWindowHandle desktopWindowHandle,
                              int x,
                              int y )
    {
      DataWindowMove data;
      data.mDesktopWindowHandle = desktopWindowHandle;
      data.mX = x;
      data.mY = y;
      QueuePush( Type::MoveWindow, &data, sizeof( data ) );
    }

    void PushEventResizeWindow( DesktopWindowHandle desktopWindowHandle,
                                int w,
                                int h )
    {
      DataWindowResize data;
      data.mDesktopWindowHandle = desktopWindowHandle;
      data.mWidth = w;
      data.mHeight = h;
      QueuePush( Type::ResizeWindow, &data, sizeof( data ) );
    }

    void PushEventMouseWheel(  int ticks )
    {
      DataMouseWheel data;
      data.mDelta = ticks;
      QueuePush( Type::MouseWheel, &data, sizeof( data ) );
    }

    void PushEventKeyState( Key key, bool down )
    {
      DataKeyState data;
      data.mDown = down;
      data.mKey = key;
      QueuePush( Type::KeyState, &data, sizeof( data ) );
    }

    void PushEventKeyInput( Codepoint codepoint )
    {
      DataKeyInput data;
      data.mCodepoint = codepoint;
      QueuePush( Type::KeyInput, &data, sizeof( data ) );

    }

    static int GetIUnusedDesktopWindow()
    {
      for( int i = 0; i < kMaxDesktopWindowStateCount; ++i )
        if( !gDesktopWindowStates[ i ].mDesktopWindowHandle.IsValid() )
          //if( !gDesktopWindowStates[ i ].mNativeWindowHandle )
          return i;
      TAC_INVALID_CODE_PATH;
      return -1;
    }

    void ProcessStuff()
    {
      for( ;; )
      {
        DesktopEvent::Type desktopEventType;
        if( !DesktopEvent::QueuePop( &desktopEventType ) )
          break;

        switch( desktopEventType )
        {
          case DesktopEvent::Type::CreateWindow:
          {
            DesktopEvent::DataCreateWindow data;
            DesktopEvent::QueuePop( &data, sizeof( data ) );

            const int iCreatedWindow = GetIUnusedDesktopWindow();
            DesktopWindowState* desktopWindowState = &gDesktopWindowStates[ iCreatedWindow ];
            desktopWindowState->mWidth = data.mWidth;
            desktopWindowState->mHeight = data.mHeight;
            desktopWindowState->mDesktopWindowHandle = data.mDesktopWindowHandle;
            desktopWindowState->mX = data.mX;
            desktopWindowState->mY = data.mY;
          } break;
          case DesktopEvent::Type::CursorUnobscured:
          {
            DesktopEvent::DataCursorUnobscured data;
            DesktopEvent::QueuePop( &data, sizeof( data ) );
            for( DesktopWindowState& state : gDesktopWindowStates )
            {
              const bool unobscured =
                state.mDesktopWindowHandle.mIndex ==
                data.mDesktopWindowHandle.mIndex;
              state.mCursorUnobscured = unobscured;
            }
          } break;
          case DesktopEvent::Type::MoveWindow:
          {
            DesktopEvent::DataWindowMove data;
            DesktopEvent::QueuePop( &data, sizeof( data ) );
            DesktopWindowState* state = FindDesktopWindowState( data.mDesktopWindowHandle );
            state->mX = data.mX;
            state->mY = data.mY;
          } break;
          case DesktopEvent::Type::ResizeWindow:
          {
            DesktopEvent::DataWindowResize data;
            DesktopEvent::QueuePop( &data, sizeof( data ) );
            DesktopWindowState* state = FindDesktopWindowState( data.mDesktopWindowHandle );
            state->mWidth = data.mWidth;
            state->mHeight = data.mHeight;
          } break;
          case DesktopEvent::Type::KeyInput:
          {
            DesktopEvent::DataKeyInput data;
            DesktopEvent::QueuePop( &data, sizeof( data ) );
            KeyboardInput::Instance->mWMCharPressedHax = data.mCodepoint;
          } break;
          case DesktopEvent::Type::KeyState:
          {
            DesktopEvent::DataKeyState data;
            DesktopEvent::QueuePop( &data, sizeof( data ) );
            KeyboardInput::Instance->SetIsKeyDown( data.mKey, data.mDown );
          } break;
          case DesktopEvent::Type::MouseWheel:
          {
            DesktopEvent::DataMouseWheel data;
            DesktopEvent::QueuePop( &data, sizeof( data ) );
            KeyboardInput::Instance->mCurr.mMouseScroll += data.mDelta;
          } break;
          TAC_INVALID_DEFAULT_CASE( desktopEventType );
        }
      }
    }
  }
}
