#include "tac_render_material_cache.h" // self-inc

namespace Tac::Render
{

  void            RenderMaterialCache::Clear()
  {
    for( RenderMaterial& renderMaterial : mRenderMaterials )
      renderMaterial.Uninit();

    mRenderMaterials = {};
  }

  RenderMaterial* RenderMaterialCache::Find( const Material* material )
  {
    const AssetPathStringView shaderGraph{ material->mShaderGraph };
    const HashValue hashValue{ Hash( ( StringView )shaderGraph ) };
    for ( RenderMaterial& renderMaterial : mRenderMaterials )
    {
      if ( renderMaterial.mMaterialShaderPathHash == hashValue &&
        ( AssetPathStringView )renderMaterial.mShaderGraphPath == shaderGraph )
      {
        return &renderMaterial;
      }
    }

    return nullptr;
  }
}
