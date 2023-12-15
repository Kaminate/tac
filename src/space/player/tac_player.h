
#pragma once
#include "space/tac_space_types.h"
#include "space/tac_space.h"
#include "src/common/dataprocess/tac_serialization.h"
#include "src/common/preprocess/tac_preprocessor.h"
#include "src/common/math/tac_vector2.h"
#include "src/common/math/tac_vector3.h"

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

