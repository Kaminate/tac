#pragma once

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"

namespace Tac
{
  void CreationUpdateAssetView( World*, Camera*);
  void CreationRenderAssetView( Errors& );
}

