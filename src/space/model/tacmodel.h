
#pragma once

#include "src/space/tacComponent.h"
#include "src/common/containers/tacVector.h"
#include "src/common/tacString.h"

namespace Tac
{

struct Mesh;

struct Model : public Component
{
  static void SpaceInitGraphicsModel();

  static Model* GetModel( Entity* );
  static const Model* GetModel( const Entity* );
  static ComponentRegistryEntry* ModelComponentRegistryEntry;
  ComponentRegistryEntry* GetEntry() override;

  //TextureUUID mTextureUUID = NullTextureUUID;
  //GeometryUUID mGeometryUUID = NullGeometryUUID;
  v3 mColorRGB = { 1, 1, 1 };
  String mGLTFPath;
  Mesh* mesh = nullptr;
};

const Vector< NetworkBit > ComponentModelBits = [](){
  Vector< NetworkBit > networkBits;
  //NetworkBit( "Model::mTextureUUID", OffsetOf( Model, mTextureUUID ), UUIDFormat, NoMaxEnumValue ),
  //NetworkBit( "Model::mGeometryUUID", OffsetOf( Model, mGeometryUUID ), UUIDFormat, NoMaxEnumValue ),
  return networkBits;
}();


}

