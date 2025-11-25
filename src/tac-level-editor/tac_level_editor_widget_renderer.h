// The widget renderer is responsibile for rendering the widgets of the gizmo manager
#pragma once

#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-level-editor/tac_level_editor_mouse_picking.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/error/tac_error_handling.h"

#define TAC_IS_WIDGET_RENDERER_ENABLED() 1

#if TAC_IS_WIDGET_RENDERER_ENABLED()

namespace Tac
{
  struct WidgetRenderer
  {
    static void Init( Errors& );
    static void Uninit();
    static void RenderTranslationWidget( Render::IContext*, WindowHandle, const Camera*, Errors& );
  };
}
#endif
