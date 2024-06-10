#pragma once

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"

namespace Tac
{
  struct IconRenderer
  {
    void Init( float, Errors& );
    void Uninit();

    void RenderLights( const World*,
                       const Camera*,
                       Render::IContext*,
                       WindowHandle,
                       Errors& );

  private:
    Render::PipelineParams GetPipelineParams();
    void UpdatePerFrame( Render::IContext*, WindowHandle, const Camera* , Errors&);
    void UpdatePerObj( Render::IContext*, const Light* , Errors&);

    Render::ProgramHandle         mSpriteShader             {};
    Render::PipelineHandle        mSpritePipeline           {};
    Render::BufferHandle          mPerFrame                 {};
    Render::BufferHandle          mPerObj                   {};
    Render::SamplerHandle         mSampler                  {};

    Render::IShaderVar*           mShaderPerFrame           {};
    Render::IShaderVar*           mShaderPerObj             {};
    Render::IShaderVar*           mShaderTexture            {};
    Render::IShaderVar*           mShaderSampler            {};
  };
}
