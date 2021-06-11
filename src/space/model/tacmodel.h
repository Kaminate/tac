
#pragma once

#include "src/space/tacComponent.h"
#include "src/common/containers/tacVector.h"
#include "src/common/string/tacString.h"

namespace Tac
{

  struct Mesh;

  struct Model : public Component
  {
    static Model*                        GetModel( Entity* );
    static const Model*                  GetModel( const Entity* );
    const ComponentRegistryEntry*        GetEntry() const override;
    //static const ComponentRegistryEntry* GetEntryStatic();
    //TextureUUID                        mTextureUUID = NullTextureUUID;
    //GeometryUUID                       mGeometryUUID = NullGeometryUUID;
    v3                                   mColorRGB = { 1, 1, 1 };
    String                               mModelPath;
    int                                  mModelIndex = -1;
    //Mesh*                                mesh = nullptr;
    //bool                                 mTryingNewThing = false;
  };

  void                             RegisterModelComponent();

  //const Vector< NetworkBit > ComponentModelBits = []() {
  //  Vector< NetworkBit > networkBits;
  //  //NetworkBit( "Model::mTextureUUID", OffsetOf( Model, mTextureUUID ), UUIDFormat, NoMaxEnumValue ),
  //  //NetworkBit( "Model::mGeometryUUID", OffsetOf( Model, mGeometryUUID ), UUIDFormat, NoMaxEnumValue ),
  //  return networkBits;
  //}( );


}

