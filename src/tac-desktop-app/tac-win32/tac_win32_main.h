#pragma once

#include "tac-engine-core/system/tac_platform.h"

namespace Tac
{
  void RedirectStreamBuf();

  struct Win32PlatformFns : public PlatformFns
  {
    void PlatformImGui( Errors& ) override;
    void PlatformFrameBegin( Errors& ) override;
    void PlatformFrameEnd( Errors& ) override;
    void PlatformSpawnWindow( const PlatformSpawnWindowParams&, Errors& ) override;
    void PlatformDespawnWindow( const WindowHandle& ) override;
    void PlatformWindowMoveControls( const WindowHandle&,
                                     const DesktopWindowRect& ) override;
    void PlatformWindowResizeControls( const WindowHandle&, int ) override;
    WindowHandle PlatformGetMouseHoveredWindow() override;
  };
} // namespace

