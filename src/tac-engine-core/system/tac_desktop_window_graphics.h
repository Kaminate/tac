#pragma once

#include "tac-engine-core/system/tac_desktop_window.h"
#include "tac-std-lib/string/tac_string_view.h"

namespace Tac::Render { struct FramebufferHandle; struct ViewHandle; }

namespace Tac
{
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
