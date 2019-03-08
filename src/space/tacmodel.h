#pragma once

#include "taccommon.h"
#include "taccomponent.h"
#include "graphics\tacRenderer.h"
#include "common\tacAssetManager.h"

#include <string>

struct TacModel : public TacComponent
{
  void TacDebugImgui() override;
  void TacDebugImguiChangeModel();
  void TacDebugImguiChangeTexture();
  TacComponentType GetComponentType() override { return TacComponentType::Model; };

  TacTextureUUID mTextureUUID = TacNullTextureUUID;
  TacGeometryUUID mGeometryUUID = TacNullGeometryUUID;
};

const std::vector< TacNetworkBit > TacComponentModelBits =
{
  TacNetworkBit( "TacModel::mTextureUUID", TacOffsetOf( TacModel, mTextureUUID ), TacUUIDFormat, TacNoMaxEnumValue ),
  TacNetworkBit( "TacModel::mGeometryUUID", TacOffsetOf( TacModel, mGeometryUUID ), TacUUIDFormat, TacNoMaxEnumValue ),
};
