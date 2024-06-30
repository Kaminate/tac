#pragma once

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"

#define TAC_RADIOSITY_BAKE_PRESENTATION_ENABLED() 1

#if TAC_RADIOSITY_BAKE_PRESENTATION_ENABLED()

namespace Tac
{

  struct RadiosityBakePresentation
  {
    static void Init( Errors& );
    static void Render( Render::IContext*,
                        const World*,
                        const Camera*,
                        Errors& );
    static void Uninit();
    static void DebugImGui();
  };
}

#endif()

