#pragma once

#include "tac-ecs/component/tac_component.h"
#include "tac-std-lib/meta/tac_meta_decl.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-engine-core/asset/tac_asset.h"

namespace Tac
{
  struct Sprite3D : public Component
  {
    static void RegisterComponent();
    static auto GetSprite3D( dynmc Entity* ) -> dynmc Sprite3D*;
    static auto GetSprite3D( const Entity* ) -> const Sprite3D*;
    auto GetEntry() const -> const ComponentInfo* override;

    AssetPathString mTexturePath  {};
  };


  TAC_META_DECL( Sprite3D );
}

