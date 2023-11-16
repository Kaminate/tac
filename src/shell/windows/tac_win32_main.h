#pragma once
#include "src/shell/tac_desktop_app.h"

namespace Tac
{
  struct Win32PlatformFns : public PlatformFns
  {
    void PlatformImGui( Errors& ) const override;
    void PlatformFrameBegin( Errors& ) const override;
    void PlatformFrameEnd( Errors& ) const override;
    void PlatformSpawnWindow( const SpawnWindowParams&, Errors& ) const override;
    void PlatformDespawnWindow( const DesktopWindowHandle& ) const override;
    void PlatformWindowMoveControls( const DesktopWindowHandle&,
                                     const DesktopWindowRect& ) const override;
    void PlatformWindowResizeControls( const DesktopWindowHandle&, int ) const override;
    DesktopWindowHandle PlatformGetMouseHoveredWindow() const override;
  };
}
