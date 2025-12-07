#pragma once

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"

#define TAC_IS_ICON_RENDERER_ENABLED() 1
#if TAC_IS_ICON_RENDERER_ENABLED()

namespace Tac
{
  struct IconRenderer
  {
    static void Init( Errors& );
    static void Uninit();
    static void RenderIcons( const World*, const Camera*, Render::IContext*, WindowHandle, Errors& );
  };
}
#endif
