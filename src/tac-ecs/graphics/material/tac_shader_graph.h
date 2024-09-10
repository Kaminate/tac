#pragma once

#include "tac-ecs/graphics/material/tac_material_input.h"
#include "tac-ecs/graphics/material/tac_material_input_layout.h"
#include "tac-engine-core/asset/tac_asset.h"

namespace Tac
{
  struct ShaderGraph
  {
    static Json          ToJson( const ShaderGraph& );
    static ShaderGraph   FromJson( const Json& );

    static void          ToPath( const ShaderGraph&, AssetPathStringView, Errors& );
    static ShaderGraph   FromPath( AssetPathStringView, Errors& );

    MaterialInputLayout  mVertexShaderOutput {};
    MaterialInput        mMaterialInputs     {};
    String               mMaterialShader     {};
  };
} // namespace Tac

