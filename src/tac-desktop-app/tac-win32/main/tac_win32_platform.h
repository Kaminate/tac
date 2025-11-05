#pragma once

#include "tac-engine-core/platform/tac_platform.h"

namespace Tac
{
  struct Win32PlatformFns : public PlatformFns
  {
    void PlatformImGui( Errors& ) const override;
    void PlatformFrameBegin( Errors& ) const override;
    void PlatformFrameEnd( Errors& ) const override;
    void PlatformSpawnWindow( const PlatformSpawnWindowParams&, Errors& ) const override;
    void PlatformDespawnWindow( WindowHandle ) const override;
    void PlatformSetWindowPos( WindowHandle, v2i ) const override;
    void PlatformSetWindowSize( WindowHandle, v2i ) const override;
    void PlatformSetMouseCursor( PlatformMouseCursor ) const override;
    auto PlatformGetMouseHoveredWindow() const -> WindowHandle override;
  };
} // namespace

