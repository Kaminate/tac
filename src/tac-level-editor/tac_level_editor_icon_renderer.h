#pragma once

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-ecs/world/tac_world.h"

namespace Tac
{
  struct IconRenderer
  {
    void Init( float, Errors& );
    void Uninit();

    void RenderLights( const World*,
                       Render::IContext*,
                       WindowHandle,
                       Errors& );


    Render::ProgramHandle         mSpriteShader             {};
    Render::PipelineHandle        mSpritePipeline           {};
    float                         mLightWidgetSize          {};
  };
}
