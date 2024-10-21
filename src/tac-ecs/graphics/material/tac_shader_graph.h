#pragma once

#include "tac-ecs/graphics/material/tac_material_input.h"
#include "tac-ecs/graphics/material/tac_material_vs_out.h"
#include "tac-engine-core/asset/tac_asset.h"

namespace Tac
{
  // The ShaderGraph defines the inputs that need to be bound to the shader program
  //
  // For example, a shader graph may say that the shader needs slot 0 to be a spec
  // texture, but which texture to put in that slot is put in the Tac::Material
  // 
  // $$$ todo describe
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

