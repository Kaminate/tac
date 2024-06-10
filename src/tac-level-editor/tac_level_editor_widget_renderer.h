#pragma once

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-ecs/world/tac_world.h"

namespace Tac
{
  struct WidgetRenderer
  {
    void Init( Errors& );
    void Uninit();

    void RenderTranslationWidget( Render::IContext*,
                                       WindowHandle,
                                       Errors& );

  private:

    Render::ProgramHandle         m3DShader                 {};
    Render::PipelineHandle        m3DPipeline               {};
  };
}
