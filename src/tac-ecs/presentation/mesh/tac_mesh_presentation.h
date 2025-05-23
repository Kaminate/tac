#pragma once

#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/graphics/tac_graphics.h"

#define TAC_MESH_PRESENTATION_ENABLED() 1

#if TAC_MESH_PRESENTATION_ENABLED()

namespace Tac
{
  struct MeshPresentation
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
    static const Mesh* GetModelMesh( const Model*, Errors& );
  };
}

#endif
