#pragma once

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"

namespace Tac
{
  struct CreationAssetView
  {
    static void Update( World*, Camera*, Errors& );
    static void Render( Errors& );
    static bool sShowWindow;
  };
}

