#pragma once

#include "tac-engine-core/platform/tac_platform.h"

namespace Tac
{
  void RedirectStreamBuf();

  struct Win32PlatformFns : public PlatformFns
  {
    void PlatformImGui( Errors& ) const override;
    void PlatformFrameBegin( Errors& ) const override;
    void PlatformFrameEnd( Errors& ) const override;
    void PlatformSpawnWindow( const PlatformSpawnWindowParams&, Errors& ) const override;
    void PlatformDespawnWindow( WindowHandle ) const override;
  };
} // namespace

