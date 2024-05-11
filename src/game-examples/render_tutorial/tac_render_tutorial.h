#pragma once

#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-desktop-app/desktop_window/tac_desktop_window_settings_tracker.h"
#include "tac-std-lib/math/tac_vector3.h"

namespace Tac
{
  WindowHandle RenderTutorialCreateWindow( const SysWindowApi*, StringView, Errors& );

  struct ClipSpacePosition3
  {
    ClipSpacePosition3() = default;
    explicit ClipSpacePosition3( v3 v ) : mValue( v ) {}
    explicit ClipSpacePosition3( float x, float y, float z ) : mValue{ x, y, z } {}
    v3 mValue;
  };

  struct LinearColor3
  {
    LinearColor3() = default;
    explicit LinearColor3( v3 v ) : mValue( v ) {}
    explicit LinearColor3( float x, float y, float z ) : mValue{ x, y, z } {}
    v3 mValue;
  };

  struct TextureCoordinate2
  {
    TextureCoordinate2() = default;
    explicit TextureCoordinate2( v2 v ) : mValue( v ) {}
    explicit TextureCoordinate2( float x, float y ) : mValue{ x, y } {}
    v2 mValue;
  };


} // namespace Tac

