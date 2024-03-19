#pragma once

#include "tac-engine-core/system/tac_desktop_window.h"

namespace Tac { struct Errors; }
namespace Tac
{

  struct PlatformSpawnWindowParams
  {
    DesktopWindowHandle mHandle;
    const char*         mName;
    int                 mX;
    int                 mY;
    int                 mWidth;
    int                 mHeight;
  };

  struct PlatformFns
  {
    virtual void PlatformImGui( Errors& ) {}
    virtual void PlatformFrameBegin( Errors& ) {}
    virtual void PlatformFrameEnd( Errors& ) {}
    virtual void PlatformSpawnWindow( const PlatformSpawnWindowParams&, Errors& ) {}
    virtual void PlatformDespawnWindow( const DesktopWindowHandle& ) {}
    virtual void PlatformSetWindowPos( const DesktopWindowHandle&, int , int  ) {}
    virtual void PlatformSetWindowSize( const DesktopWindowHandle&, int , int  ) {}
    virtual void PlatformWindowMoveControls( const DesktopWindowHandle&,
                                             const DesktopWindowRect& ) {}
    virtual void PlatformWindowResizeControls( const DesktopWindowHandle&, int ) {}
    virtual DesktopWindowHandle PlatformGetMouseHoveredWindow() { return{}; }

    static PlatformFns* GetInstance();
    static void         SetInstance( PlatformFns* );
  };

} // namespace Tac