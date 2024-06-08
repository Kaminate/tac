#pragma once

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-rhi/render3/tac_render_api.h"

namespace Tac
{
  struct IconRenderer
  {
    void Init( Errors& );

    Render::ProgramHandle         mSpriteShader             {};
    Render::PipelineHandle        mSpritePipeline           {};
  };
}
