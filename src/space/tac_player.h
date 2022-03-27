
#pragma once
#include "src/space/tac_space_types.h"
#include "src/common/tac_serialization.h"
#include "src/common/tac_preprocessor.h"
#include "src/common/math/tac_vector2.h"

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

