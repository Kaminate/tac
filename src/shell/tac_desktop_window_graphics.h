#pragma once

#include "tac-std-lib/system/tac_desktop_window.h"

namespace Tac
{
  namespace Render
  {
    struct FramebufferHandle;
    struct ViewHandle;
  }

  struct WindowGraphics
  {
    struct NativeHandleChangedData
    {
       DesktopWindowHandle mDesktopWindowHandle;
       const void*         mNativeWindowHandle;
       StringView          mName;
       int                 mW;
       int                 mH;
    };

    Render::FramebufferHandle GetFramebuffer( const DesktopWindowHandle& );
    Render::ViewHandle        GetView( const DesktopWindowHandle& );
    void                      NativeHandleChanged( const NativeHandleChangedData& );
    void                      Resize( const DesktopWindowHandle&, int w, int h );

    static WindowGraphics& Instance();
  };

}
