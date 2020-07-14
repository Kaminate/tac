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

    }

    void PushEventResizeWindow( DesktopWindowHandle desktopWindowHandle,
                                int w,
                                int h )
    {

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

    void ProcessStuff( bool* createdWindows )
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
            //desktopWindowState->mNativeWindowHandle = data.mNativeWindowHandle;
            desktopWindowState->mDesktopWindowHandle = data.mDesktopWindowHandle;
            desktopWindowState->mX = data.mX;
            desktopWindowState->mY = data.mY;
            createdWindows[ iCreatedWindow ] = true;
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
        }
      }
    }
  }
}
