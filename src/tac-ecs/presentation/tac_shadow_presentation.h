#pragma once

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-ecs/graphics/tac_graphics.h"

namespace Tac
{
  struct ShadowPresentation
  {
    static void Init( Errors& );
    static void Uninit();
    static void Render( World*, Errors& );
    static void DebugImGui();
  };
}

