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

    void QueuePush( Type type )
    {
      QueuePush( &type, sizeof( Type ) );
    }
    void QueuePush( void* bytes, int byteCount )
    {
      std::lock_guard< std::mutex > lockGuard( gMutex );
      const int gQueuePreTail = gQueueTail + byteCount > kQueueCapacity ? 0 : gQueueTail;
      MemCpy( gQueue + gQueuePreTail, bytes, byteCount );
      gQueueTail = gQueuePreTail + byteCount;
    }
    bool QueuePop( Type* type )
    {
      return QueuePop( type, sizeof( Type ) );
    }
    bool QueuePop( void* bytes, int byteCount )
    {
      std::lock_guard< std::mutex > lockGuard( gMutex );
      if( gQueueHead == gQueueTail )
        return false;
      const int gQueuePreHead = gQueueHead + byteCount > kQueueCapacity ? 0 : gQueueHead;
      MemCpy( bytes, gQueue + gQueuePreHead, byteCount );
      gQueueHead = gQueuePreHead + byteCount;
      return true;
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

    
    ProcessStuffOutput ProcessStuff()
    {
      ProcessStuffOutput result;

      for( ;; )
      {
        DesktopEvent::Type desktopEventType;
        if( !DesktopEvent::QueuePop( &desktopEventType ) )
          return result;

        switch( desktopEventType )
        {
          case DesktopEvent::Type::CreateWindow:
          {
            DesktopEvent::DataCreateWindow data;
            DesktopEvent::QueuePop( &data, sizeof( data ) );
            result.mCreatedWindow = true;
            result.mCreatedWindowState.mDesktopWindowHandle = data.mDesktopWindowHandle;
            result.mCreatedWindowState.mWidth = data.mWidth;
            result.mCreatedWindowState.mHeight = data.mHeight;
            result.mCreatedWindowState.mNativeWindowHandle = data.mNativeWindowHandle;
          } return result; // not break!
        }
      }
      return result;
    }
  }
}
