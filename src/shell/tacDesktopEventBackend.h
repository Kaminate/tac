#pragma once
#include "src/common/tacDesktopWindow.h"

namespace Tac
{
  namespace DesktopEvent
  {
    struct DataCreateWindow
    {
      DesktopWindowHandle mDesktopWindowHandle;
      int                 mWidth;
      int                 mHeight;
      void*               mNativeWindowHandle;
    };

    struct DataCursorUnobscured
    {
      DesktopWindowHandle mDesktopWindowHandle;
    };

    enum class Type
    {
      CreateWindow,
      CursorUnobscured,
    };

    void                  QueuePush( Type, void* bytes, int byteCount );
    bool                  QueuePop( Type* type, void* bytes, int byteCount );
  }
}
