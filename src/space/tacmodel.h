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
  void TacDebugImgui() override;
  void TacDebugImguiChangeModel();
  void TacDebugImguiChangeTexture();
  TacComponentType GetComponentType() override { return TacComponentType::Model; };

  //TacTextureUUID mTextureUUID = TacNullTextureUUID;
  //TacGeometryUUID mGeometryUUID = TacNullGeometryUUID;
  TacString mGLTFPath;
  TacMesh* mesh = nullptr;
};

const TacVector< TacNetworkBit > TacComponentModelBits = [](){
  TacVector< TacNetworkBit > networkBits;
  //TacNetworkBit( "TacModel::mTextureUUID", TacOffsetOf( TacModel, mTextureUUID ), TacUUIDFormat, TacNoMaxEnumValue ),
  //TacNetworkBit( "TacModel::mGeometryUUID", TacOffsetOf( TacModel, mGeometryUUID ), TacUUIDFormat, TacNoMaxEnumValue ),
  return networkBits;
}();

