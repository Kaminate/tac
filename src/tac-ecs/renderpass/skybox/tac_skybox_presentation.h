#pragma once

#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-ecs/world/tac_world.h"

#define TAC_SKYBOX_PRESENTATION_ENABLED() 1

#if TAC_SKYBOX_PRESENTATION_ENABLED()

namespace Tac
{
  struct SkyboxPresentation
  {

    static void Init( Errors& );
    static void Uninit();
    static void Render( Render::IContext*,
                                   const World*,
                                   const Camera*,
                                   const v2i viewSize,
                                   const Render::TextureHandle viewId,
                                   Errors& );

    static void DebugImGui();
  };
}

#endif
