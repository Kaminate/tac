#pragma once

#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/window/tac_window_handle.h"

namespace Tac
{
  auto RenderTutorialCreateWindow( StringView, Errors& ) -> WindowHandle;

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

