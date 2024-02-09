#include "space/net/tac_player_diff.h" // self-inc

#include "common/error/tac_error_handling.h"
//#include "space/ecs/tac_component_registry.h" // GetIndex
//#include "space/ecs/tac_entity.h" // GetComponent
#include "space/player/tac_player.h"
#include "space/world/tac_world.h"
//#include "space/net/tac_space_net.h" // GetNetworkBitfield

namespace Tac
{
  // ----------------------------------------------------------------------------------------------

  void PlayerDiffs::Write( World* oldWorld, World* newWorld, Writer* writer )
  {
    PlayerDiffs playerDiffs( oldWorld, newWorld );
    playerDiffs.Write( writer );
  }

  PlayerDiffs::PlayerDiffs( World* oldWorld, World* newWorld )
  {
    for( Player* oldPlayer : oldWorld->mPlayers )
      if( !newWorld->FindPlayer( oldPlayer->mPlayerUUID ) )
        deletedPlayerUUIDs.push_back( oldPlayer->mPlayerUUID );

    for( Player* newPlayer : newWorld->mPlayers )
    {
      const PlayerUUID playerUUID = newPlayer->mPlayerUUID;
      const Player* oldPlayer = oldWorld->FindPlayer( playerUUID );

      const NetBitDiff bitfield = GetNetworkBitfield( oldPlayer, newPlayer, PlayerNetworkBitsGet() );
      if( bitfield.Empty() )
        continue;

      const PlayerDifference diff
      {
        .mBitfield = bitfield,
        .mNewPlayer = newPlayer,
        .playerUUID = playerUUID,
      };
      oldAndNewPlayers.push_back( diff );
    }

  }

  void PlayerDiffs::Write( Writer* writer )
  {
    writer->Write( ( PlayerCount )deletedPlayerUUIDs.size() );
    for( PlayerUUID playerUUID : deletedPlayerUUIDs )
      writer->Write( playerUUID );

    writer->Write( ( PlayerCount )oldAndNewPlayers.size() );
    for( PlayerDifference& diff : oldAndNewPlayers )
    {
      writer->Write( diff.playerUUID );
      writer->Write( diff.mNewPlayer, diff.mBitfield, PlayerNetworkBitsGet() );
    }
  }


  void PlayerDiffs::Read( World* world, Reader* reader, Errors& errors )
  {

    const auto numPlayerDeleted = TAC_CALL( reader->Read<PlayerCount>( errors ) );

    for( PlayerCount i = 0; i < numPlayerDeleted; ++i )
    {
      const auto deletedPlayerUUID = TAC_CALL( reader->Read<PlayerUUID >( errors ) );
      world->KillPlayer( deletedPlayerUUID );
    }

    const auto numPlayersDifferent = TAC_CALL(reader->Read<PlayerCount>(errors));

    for( PlayerCount i = 0; i < numPlayersDifferent; ++i )
    {
      const auto differentPlayerUUID = TAC_CALL( reader->Read<PlayerUUID>( errors ) );

      Player* player = world->FindPlayer( differentPlayerUUID );
      if( !player )
        player = world->SpawnPlayer( differentPlayerUUID );

      TAC_RAISE_ERROR_IF( !reader->Read( player, PlayerNetworkBitsGet() ),
                          "failed to read player bits" );
    }

  }

  // ----------------------------------------------------------------------------------------------


} // namespace Tac



