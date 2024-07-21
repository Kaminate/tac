#include "tac_render_material.h" // self-inc

namespace Tac::Render
{
  RenderMaterial* MaterialManager::GetRenderMaterial( StringView materialShader )
  {
    HashValue hashValue{ Hash( materialShader ) };
    for ( RenderMaterial& renderMaterial : mRenderMaterials )
    {
      if ( renderMaterial.mMaterialShaderHash == hashValue &&
        ( StringView )renderMaterial.mMaterialShader == materialShader )
      {
        return &renderMaterial;
      }
    }

    Render::PipelineHandle        meshPipeline;
    Vector< Render::IShaderVar* > shaderVars;

    mRenderMaterials.resize( mRenderMaterials.size() + 1 );
    RenderMaterial& renderMaterial{ mRenderMaterials.back() };
    renderMaterial = RenderMaterial
    {
      .mMaterialShader     { materialShader },
      .mMaterialShaderHash { hashValue },
      .mMeshPipeline       { meshPipeline },
      .mShaderVars         { shaderVars },
    };

    return &renderMaterial;
  }

} // namespace Tac::Render
