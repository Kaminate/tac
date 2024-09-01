#pragma once

#include "tac-ecs/world/tac_world.h"
#include "tac-std-lib/dataprocess/tac_serialization.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
  struct EntityDiffAPI
  {
    static void Write( WorldsToDiff, WriteStream* );
    static void Read( World* , ReadStream* , Errors& );
  };

} // namespace Tac

