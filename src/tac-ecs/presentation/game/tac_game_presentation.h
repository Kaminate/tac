#pragma once

#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/graphics/tac_graphics.h"

#define TAC_GAME_PRESENTATION_ENABLED() 1

#if TAC_GAME_PRESENTATION_ENABLED()

namespace Tac
{
  struct GamePresentation
  {
    struct RenderParams
    {
      Render::IContext*     mContext            {};
      const World*          mWorld              {};
      const Camera*         mCamera             {};
      v2i                   mViewSize           {};
      Render::TextureHandle mColor              {};
      Render::TextureHandle mDepth              {};
      Debug3DDrawBuffers*   mBuffers            {};
      bool                  mIsLevelEditorWorld {};
    };

    static void Init( Errors& );
    static void Uninit();
    static void Render(RenderParams, Errors& );
    static void DebugImGui( Graphics* );
  };
}

#endif
