#pragma once

#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/net/tac_space_net.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-std-lib/dataprocess/tac_serialization.h"

namespace Tac
{
  struct PlayerDifference
  {
    NetBitDiff mBitfield  {};
    Player*    mNewPlayer {};
    PlayerUUID playerUUID {};
  };

  struct PlayerDiffs
  {
    static void Write(  World* oldWorld, World* newWorld , WriteStream* );
    static void Read( World* world, ReadStream* reader, Errors& errors );

  private:
    PlayerDiffs( World* oldWorld, World* newWorld );
    void Write( WriteStream* );

    Vector< PlayerUUID >       deletedPlayerUUIDs;
    Vector< PlayerDifference > oldAndNewPlayers;
  };

} // namespace Tac

