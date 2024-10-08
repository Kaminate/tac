#include "tac_player_diff.h" // self-inc

#include "tac-std-lib/error/tac_error_handling.h"
//#include "tac-ecs/component/tac_component_registry.h" // GetIndex
//#include "tac-ecs/entity/tac_entity.h" // GetComponent
#include "tac-ecs/player/tac_player.h"
#include "tac-ecs/world/tac_world.h"
//#include "tac-ecs/net/tac_space_net.h" // GetNetVarfield

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
      const PlayerUUID playerUUID { newPlayer->mPlayerUUID };
      const Player* oldPlayer { oldWorld->FindPlayer( playerUUID ) };

      const NetBitDiff bitfield { GetNetVarfield( oldPlayer, newPlayer, PlayerNetVarsGet() ) };
      if( bitfield.Empty() )
        continue;

      const PlayerDifference diff
      {
        .mBitfield  { bitfield },
        .mNewPlayer { newPlayer },
        .playerUUID { playerUUID },
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
      writer->Write( diff.mNewPlayer, diff.mBitfield, PlayerNetVarsGet() );
    }
  }


  void PlayerDiffs::Read( World* world, Reader* reader, Errors& errors )
  {

    TAC_CALL( const auto numPlayerDeleted{ reader->Read<PlayerCount>( errors ) } );

    for( PlayerCount i{}; i < numPlayerDeleted; ++i )
    {
      TAC_CALL( const auto deletedPlayerUUID{ reader->Read<PlayerUUID >( errors ) } );
      world->KillPlayer( deletedPlayerUUID );
    }

    TAC_CALL( const auto numPlayersDifferent{ reader->Read<PlayerCount>( errors ) } );

    for( PlayerCount i {  }; i < numPlayersDifferent; ++i )
    {
      TAC_CALL( const auto differentPlayerUUID{ reader->Read<PlayerUUID>( errors ) } );

      Player* player { world->FindPlayer( differentPlayerUUID ) };
      if( !player )
        player = world->SpawnPlayer( differentPlayerUUID );

      TAC_RAISE_ERROR_IF( !reader->Read( player, PlayerNetVarsGet() ),
                          "failed to read player bits" );
    }

  }

  // ----------------------------------------------------------------------------------------------


} // namespace Tac



