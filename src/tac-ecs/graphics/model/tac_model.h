#pragma once

#include "tac-ecs/component/tac_component.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-engine-core/asset/tac_asset.h"

namespace Tac
{
  struct Model : public Component
  {
    static Model*                        GetModel( Entity* );
    static const Model*                  GetModel( const Entity* );
    const ComponentRegistryEntry*        GetEntry() const override;
    //static const ComponentRegistryEntry* GetEntryStatic();
    //TextureUUID                        mTextureUUID = NullTextureUUID;
    //GeometryUUID                       mGeometryUUID = NullGeometryUUID;

    //                                   todo: rename mColor_sRGB
    //v3                                   mColorRGB   { 1, 1, 1 };
    AssetPathString                      mModelPath  {};
    int                                  mModelIndex { -1 };
    bool                                 mIsStatic   {};
  };

  void                                   RegisterModelComponent();



}

