#pragma once

#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-ecs/world/tac_world.h"

#define TAC_JPPT_PRESENTATION_ENABLED() 1

#if TAC_JPPT_PRESENTATION_ENABLED()

namespace Tac
{
  struct JPPTPresentation
  {
    static void        Init( Errors& );
    static void        Uninit();
    static void        Render( Render::IContext*,
                               const World*,
                               const Camera*,
                               v2i viewSize,
                               Render::TextureHandle color,
                               Render::TextureHandle depth,
                               Errors& );
    static void        DebugImGui();
  };
}

#endif
