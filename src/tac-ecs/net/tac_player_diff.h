#pragma once

#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/net/tac_space_net.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-std-lib/dataprocess/tac_serialization.h"

namespace Tac
{
  struct PlayerDiffs
  {
    static void Write( WorldsToDiff, WriteStream* );
    static void Read( World*, ReadStream*, Errors& );
  };

} // namespace Tac

