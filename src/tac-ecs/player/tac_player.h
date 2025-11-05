
#pragma once
#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/tac_space.h"
#include "tac-std-lib/dataprocess/tac_serialization.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/meta/tac_meta_decl.h"

namespace Tac
{
  struct Player
  {
    void       DebugImgui();
    PlayerUUID mPlayerUUID      { NullPlayerUUID };
    EntityUUID mEntityUUID      { NullEntityUUID };
    v2         mInputDirection  {};
    bool       mIsSpaceJustDown {};
    v3         mCameraPos       {};
    World*     mWorld           {};
  };

  TAC_META_DECL( Player );

  void PlayerNetVarsRegister();
  auto PlayerNetVarsGet() -> const NetVarRegistration&;

}

