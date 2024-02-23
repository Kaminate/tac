#pragma once

#include "space/tac_space_types.h"
#include "space/net/tac_space_net.h"

namespace Tac
{
  struct World;
  struct Writer;

  struct PlayerDifference
  {
    NetBitDiff mBitfield{};
    Player*    mNewPlayer = nullptr;
    PlayerUUID playerUUID{};
  };

  struct PlayerDiffs
  {
    static void Write(  World* oldWorld, World* newWorld , Writer* );
    static void Read( World* world, Reader* reader, Errors& errors );
  private:
    PlayerDiffs( World* oldWorld, World* newWorld );
    void Write( Writer* );

    Vector< PlayerUUID >       deletedPlayerUUIDs;
    Vector< PlayerDifference > oldAndNewPlayers;
  };

} // namespace Tac

