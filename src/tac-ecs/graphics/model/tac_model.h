#pragma once

#include "tac-ecs/component/tac_component.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/filesystem/tac_asset.h"

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
    v3                                   mColorRGB  { 1, 1, 1 };
    AssetPathString                      mModelPath;
    int                                  mModelIndex { -1 };
  };

  void                                   RegisterModelComponent();

  //const Vector< NetworkBit > ComponentModelBits = []() {
  //  Vector< NetworkBit > networkBits;
  //  //NetworkBit( "Model::mTextureUUID", OffsetOf( Model, mTextureUUID ), UUIDFormat, NoMaxEnumValue ),
  //  //NetworkBit( "Model::mGeometryUUID", OffsetOf( Model, mGeometryUUID ), UUIDFormat, NoMaxEnumValue ),
  //  return networkBits;
  //}( );


}

