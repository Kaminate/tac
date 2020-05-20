#include "src/shell/tacDesktopEventBackend.h"
#include "src/shell/tacDesktopApp.h"
#include "src/common/tacDesktopWindow.h" // why is this in common
#include "src/common/tacString.h"
#include <mutex>

namespace Tac
{
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

    void QueuePush( Type type, void* dataBytes, int dataByteCount )
    {
      const int totalByteCount = sizeof( Type ) + dataByteCount;
      std::lock_guard< std::mutex > lockGuard( gMutex );
      int offset = (gQueueTail + totalByteCount) > kQueueCapacity ? 0 : gQueueTail;

      MemCpy( gQueue + offset, &type, sizeof( Type ) );
      offset += sizeof( Type );

      MemCpy( gQueue + offset, dataBytes, totalByteCount );
      offset += totalByteCount;

      gQueueTail = offset;
    }

    bool QueuePop( Type* type, void* dataBytes, int dataByteCount )
    {
      std::lock_guard< std::mutex > lockGuard( gMutex );
      if( gQueueHead == gQueueTail )
        return false;
      const int totalByteCount = sizeof( Type ) + dataByteCount;
      //const int gQueuePreHead = ( gQueueHead + totalByteCount ) > kQueueCapacity ? 0 : gQueueHead;
      //MemCpy( bytes, gQueue + gQueuePreHead, byteCount );
      //gQueueHead = gQueuePreHead + byteCount;
      //return true;


      int offset = ( gQueueHead + totalByteCount ) > kQueueCapacity ? 0 : gQueueHead;

      MemCpy( type, gQueue + offset, sizeof( Type ) );
      offset += sizeof( Type );

      MemCpy( dataBytes, gQueue + offset, dataByteCount );
      offset += dataByteCount;

      gQueueHead = offset;
      return true;

    }


    void PushEventCursorUnobscured( DesktopWindowHandle desktopWindowHandle )
    {
      DataCursorUnobscured data;
      data.mDesktopWindowHandle = desktopWindowHandle;
      QueuePush( Type::CursorUnobscured );
      QueuePush( &data, sizeof( data ) );
    }

    void PushEventCreateWindow( DesktopWindowHandle desktopWindowHandle,
                                int width,
                                int height,
                                void* nativeWindowHandle )
    {
      DataCreateWindow data;
      data.mDesktopWindowHandle = desktopWindowHandle;
      data.mWidth = width;
      data.mHeight = height;
      data.mNativeWindowHandle = nativeWindowHandle;
      QueuePush( Type::CreateWindow );
      QueuePush( &data, sizeof( data ) );
    }


    DesktopWindowStates gDesktopWindowStates; // non-static

    // oh yeah this is a great file for this shit
    DesktopWindowState* FindDeskopWindowState( DesktopWindowHandle desktopWindowHandle )
    {
      
      for( DesktopWindowState& state : gDesktopWindowStates )
      {
        desktopWindowHandle.mIndex;

        if(state.mDesktopWindowHandle  )

      }

    }

    static int GetIUnusedDesktopWindow()
    {
      for( int i = 0; i < kMaxDesktopWindowStateCount; ++i )
        if( !gDesktopWindowStates[ i ].mNativeWindowHandle )
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
            desktopWindowState->mNativeWindowHandle = data.mNativeWindowHandle;
            desktopWindowState->mDesktopWindowHandle = data.mDesktopWindowHandle;
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
