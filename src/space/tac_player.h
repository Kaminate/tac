
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
    void       DebugImgui();
    PlayerUUID mPlayerUUID = NullPlayerUUID;
    EntityUUID mEntityUUID = NullEntityUUID;
    v2         mInputDirection = {};
    bool       mIsSpaceJustDown = false;
    v3         mCameraPos = {};
    World*     mWorld = nullptr;
  };

  void               PlayerNetworkBitsRegister();
  const NetworkBits& PlayerNetworkBitsGet();

}

