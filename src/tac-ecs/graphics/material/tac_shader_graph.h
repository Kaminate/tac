#pragma once

#include "tac-ecs/graphics/material/tac_material_input.h"
#include "tac-ecs/graphics/material/tac_material_vs_out.h"
#include "tac-engine-core/asset/tac_asset.h"

namespace Tac
{
  struct ShaderGraph
  {
    static Json          JsonSave( const ShaderGraph& );
    static ShaderGraph   JsonLoad( const Json& );

    static void          FileSave( const ShaderGraph&, AssetPathStringView, Errors& );
    static ShaderGraph   FileLoad( AssetPathStringView, Errors& );

    MaterialVSOut        mMaterialVSOut       {};
    MaterialInput        mMaterialInputs      {};
    String               mMaterialShader      {};
  };
} // namespace Tac

