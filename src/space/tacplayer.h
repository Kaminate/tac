
#pragma once
#include "src/space/tacSpacetypes.h"
#include "src/common/tacSerialization.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/math/tacVector2.h"

namespace Tac
{
struct World;
struct Player
{
  void DebugImgui();
  PlayerUUID mPlayerUUID = NullPlayerUUID;
  EntityUUID mEntityUUID = NullEntityUUID;
  v2 mInputDirection = {};
  bool mIsSpaceJustDown = false;
  v3 mCameraPos = {};
  World* mWorld = nullptr;
};

const Vector< NetworkBit > PlayerBits =
{
  { "mEntityUUID", TAC_OFFSET_OF( Player, mEntityUUID ), sizeof( EntityUUID ), 1 },
  { "mInputDirection", TAC_OFFSET_OF( Player, mInputDirection ), sizeof( float ), 2 },
};

}

