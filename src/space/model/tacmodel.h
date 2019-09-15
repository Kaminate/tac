#pragma once

//#include "taccommon.h"
#include "space/taccomponent.h"
#include "common/containers/tacVector.h"
#include "common/tacString.h"
//#include "graphics\tacRenderer.h"
//#include "common\tacAssetManager.h"

#include <string>

struct TacMesh;

struct TacModel : public TacComponent
{
  static void TacSpaceInitGraphicsModel();

  static TacModel* GetModel( TacEntity* );
  static const TacModel* GetModel( const TacEntity* );
  static TacComponentRegistryEntry* ModelComponentRegistryEntry;
  TacComponentRegistryEntry* GetEntry() override;

  //TacTextureUUID mTextureUUID = TacNullTextureUUID;
  //TacGeometryUUID mGeometryUUID = TacNullGeometryUUID;
  v3 mColorRGB = { 1, 1, 1 };
  TacString mGLTFPath;
  TacMesh* mesh = nullptr;
};

const TacVector< TacNetworkBit > TacComponentModelBits = [](){
  TacVector< TacNetworkBit > networkBits;
  //TacNetworkBit( "TacModel::mTextureUUID", TacOffsetOf( TacModel, mTextureUUID ), TacUUIDFormat, TacNoMaxEnumValue ),
  //TacNetworkBit( "TacModel::mGeometryUUID", TacOffsetOf( TacModel, mGeometryUUID ), TacUUIDFormat, TacNoMaxEnumValue ),
  return networkBits;
}();

