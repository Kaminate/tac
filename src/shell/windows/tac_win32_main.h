#pragma once
#include "src/shell/tac_desktop_app.h"

namespace Tac
{
  struct Win32PlatformFns : public PlatformFns
  {
    void PlatformImGui( Errors& ) override;
    void PlatformFrameBegin( Errors& ) override;
    void PlatformFrameEnd( Errors& ) override;
    void PlatformSpawnWindow( const PlatformSpawnWindowParams&, Errors& ) override;
    void PlatformDespawnWindow( const DesktopWindowHandle& ) override;
    void PlatformWindowMoveControls( const DesktopWindowHandle&,
                                     const DesktopWindowRect& ) override;
    void PlatformWindowResizeControls( const DesktopWindowHandle&, int ) override;
    DesktopWindowHandle PlatformGetMouseHoveredWindow() override;
  };
}
