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
    };

    bool                  QueuePop( Type* type );
    bool                  QueuePop( void* dataBytes, int dataByteCount );
  }
}
