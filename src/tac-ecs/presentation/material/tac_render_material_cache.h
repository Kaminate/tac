#pragma once

#include "tac-ecs/presentation/material/tac_render_material.h"


namespace Tac::Render
{
  struct RenderMaterialCache
  {
    RenderMaterial* Find( const Material* );
    void            Clear();

    Vector< RenderMaterial > mRenderMaterials;
  };
}
