#pragma once

#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/dataprocess/tac_hash.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-ecs/graphics/material/tac_material.h"
#include "tac-ecs/tac_space_types.h"

namespace Tac::Render
{
  // The renderer-side of Tac::Material
  // There is one RenderMaterial per material shader string
  struct RenderMaterial
  {
    void Uninit();

    String                        mMaterialShader;
    HashValue                     mMaterialShaderHash;
    Render::ProgramHandle         m3DShader;
    Render::PipelineHandle        mMeshPipeline;
    Render::IShaderVar*           mShaderVarPerFrame  {};
    Render::IShaderVar*           mShaderVarPerObject {};
    bool                          mAreShaderVarsSet   {};
  };

  // The instance data of RenderMaterial.
  // ie: if there are two entities that share the same material,
  //     but have different properties
  struct RenderMaterialProperties
  {
    Render::BufferHandle mMaterialData {};
    EntityUUID           mEntityUUID{ NullEntityUUID };
    HashValue            mMaterialHash; // for updating the buffer
  };

  struct RenderMaterialApi
  {
    static void            Init();
    static void            Uninit();
    static RenderMaterial* GetRenderMaterial( const Material*, Errors& );
  };
}
