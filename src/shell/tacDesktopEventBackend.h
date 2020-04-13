#pragma once
#include "src/common/tacDesktopWindow.h"

namespace Tac
{
  namespace DesktopEvent
  {
    struct DataCreateWindow
    {
      DesktopWindowHandle mDesktopWindowHandle;
      int mWidth;
      int mHeight;
      void* mNativeWindowHandle;
    };

    enum class Type
    {
      CreateWindow,

    };
    void             QueuePush( Type );
    void             QueuePush( void* bytes, int byteCount );
    bool             QueuePop( Type* type );
    bool             QueuePop( void* bytes, int byteCount );
  }
}
