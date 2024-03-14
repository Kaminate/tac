
#pragma once
#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/tac_space.h"
#include "tac-std-lib/dataprocess/tac_serialization.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector3.h"

namespace Tac
{
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

