#pragma once

#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-std-lib/math/tac_vector3.h"

namespace Tac
{
  WindowHandle RenderTutorialCreateWindow( StringView, Errors& );

  struct NDCSpacePosition3
  {
    NDCSpacePosition3() = default;
    explicit NDCSpacePosition3( v3 );
    explicit NDCSpacePosition3( float, float, float );
    v3 mValue;
  };

  struct LinearColor3
  {
    LinearColor3() = default;
    explicit LinearColor3( v3 );
    explicit LinearColor3( float, float, float );
    v3 mValue;
  };

  struct TextureCoordinate2
  {
    TextureCoordinate2() = default;
    explicit TextureCoordinate2( v2 );
    explicit TextureCoordinate2( float, float );
    v2 mValue;
  };


} // namespace Tac

