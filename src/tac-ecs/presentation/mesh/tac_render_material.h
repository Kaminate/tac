#pragma once

#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/dataprocess/tac_hash.h"
#include "tac-rhi/render3/tac_render_api.h"

namespace Tac::Render
{

  struct RenderMaterial
  {
    String                        mMaterialShader;
    HashValue                     mMaterialShaderHash;
    Render::ProgramHandle         m3DShader;
    Render::PipelineHandle        mMeshPipeline;
    Vector< Render::IShaderVar* > mShaderVars; // ?
  };

  struct MaterialManager
  {
    RenderMaterial* GetRenderMaterial( StringView materialShader );

    Vector< RenderMaterial > mRenderMaterials;
  };
}
