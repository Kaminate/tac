#pragma once
#include "tacspacetypes.h"
#include "common/tacSerialization.h"
#include "common/tacPreprocessor.h"
#include "common/math/tacVector2.h"

struct TacWorld;
struct TacPlayer
{
  void DebugImgui();
  TacPlayerUUID mPlayerUUID = TacNullPlayerUUID;
  TacEntityUUID mEntityUUID = TacNullEntityUUID;
  v2 mInputDirection = {};
  bool mIsSpaceJustDown = false;
  v3 mCameraPos = {};
  TacWorld* mWorld = nullptr;
};

const TacVector< TacNetworkBit > TacPlayerBits =
{
  { "mEntityUUID", TacOffsetOf( TacPlayer, mEntityUUID ), sizeof( TacEntityUUID ), 1 },
  { "mInputDirection", TacOffsetOf( TacPlayer, mInputDirection ), sizeof( float ), 2 },
};
