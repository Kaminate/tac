#pragma once

#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/dataprocess/tac_hash.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-ecs/graphics/material/tac_material.h"

namespace Tac::Render
{

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

  struct RenderMaterialApi
  {
    static void                      Init();
    static void                      Uninit();
    static RenderMaterial*           GetRenderMaterial( const Material*, Errors& );
    static const VertexDeclarations& GetVertexDeclarations();
  };
}
