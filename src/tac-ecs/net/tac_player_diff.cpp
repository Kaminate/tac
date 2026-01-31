#include "tac_player_diff.h" // self-inc

#include "tac-std-lib/error/tac_error_handling.h"
//#include "tac-ecs/component/tac_component_registry.h" // GetIndex
//#include "tac-ecs/entity/tac_entity.h" // GetComponent
#include "tac-ecs/player/tac_player.h"
#include "tac-ecs/world/tac_world.h"
//#include "tac-ecs/net/tac_space_net.h" // GetNetVarfield

namespace Tac
{
  struct ModifiedPlayer
  {
    Player*    mPlayer;
    NetVarDiff mDiff;
  };

  struct ModifiedPlayers
  {
    static void Read( World* world, ReadStream* reader, Errors& errors )
    {
      const NetVarRegistration& playerVars{ PlayerNetVarsGet() };

      TAC_CALL( const PlayerCount n{ reader->Read<PlayerCount>( errors ) } );

      for( PlayerCount i{}; i < n; ++i )
      {
        TAC_CALL( const PlayerUUID playerUUID{ reader->Read< PlayerUUID >( errors ) } );

        Player* player{ world->FindPlayer( playerUUID ) };
        TAC_ASSERT( player );

        TAC_CALL( playerVars.Read( reader, player, errors ) );
      }
    }
    void Write( WriteStream* writer )
    {
      const NetVarRegistration& playerVars{ PlayerNetVarsGet() };
      writer->Write( ( PlayerCount )mModifiedPlayers.size() );
      for( ModifiedPlayer& diff : mModifiedPlayers )
      {
        writer->Write( diff.mPlayer->mPlayerUUID );
        playerVars.Write( writer, diff.mPlayer, diff.mDiff );
      }
    }
    Vector< ModifiedPlayer >   mModifiedPlayers;
  };

  struct DeletedPlayers
  {
    DeletedPlayers() = default;
    DeletedPlayers( WorldsToDiff params )
    {
      World* oldWorld{ params.mOldWorld };
      World* newWorld{ params.mNewWorld};

      for( Player* oldPlayer : oldWorld->mPlayers )
      if( !newWorld->FindPlayer( oldPlayer->mPlayerUUID ) )
        mDeletedPlayerIDs.push_back( oldPlayer->mPlayerUUID );
    }
    static void Read( World* world, ReadStream* reader, Errors& errors )
    {
      TAC_CALL( const auto n{ reader->Read<PlayerCount>( errors ) } );

      for( PlayerCount i{}; i < n; ++i )
      {
        TAC_CALL( const PlayerUUID playerUUID{ reader->Read<PlayerUUID >( errors ) } );
        world->KillPlayer( playerUUID );
      }
    }
    void Write( WriteStream* writer )
    {
      writer->Write( ( PlayerCount )mDeletedPlayerIDs.size() );
      writer->WriteBytes( mDeletedPlayerIDs.data(),
                          mDeletedPlayerIDs.size() * sizeof( PlayerUUID ) );

    }
    Vector< PlayerUUID > mDeletedPlayerIDs;
  };

  struct CreatedPlayers
  {
    static void Read( World* world, ReadStream* reader, Errors& errors )
    {
      const NetVarRegistration& playerVars{ PlayerNetVarsGet() };
      TAC_CALL( const PlayerCount n{ reader->Read< PlayerCount >( errors ) } );
      for( PlayerCount i{}; i < n; ++i )
      {
        Player player{};
        TAC_CALL( playerVars.Read( reader, &player, errors ) );
        *world->SpawnPlayer( player.mPlayerUUID ) = player;
      }
    }
    void Write( WriteStream* writer )
    {
      const NetVarRegistration& playerVars{ PlayerNetVarsGet() };

      NetVarDiff diff;
      diff.SetAll();

      writer->Write( ( PlayerCount )mCreatedPlayers.size() );
      for( Player* player : mCreatedPlayers )
        playerVars.Write( writer, player, diff );
    }
    Vector< Player* >          mCreatedPlayers;
  };


  struct PlayerDiffsAux
  {
    PlayerDiffsAux( WorldsToDiff params )
    {
      mDeletedPlayers = DeletedPlayers( params );
      World* oldWorld{ params.mOldWorld };
      World* newWorld{ params.mNewWorld };

      for( Player* newPlayer : newWorld->mPlayers )
      {
        const PlayerUUID playerUUID { newPlayer->mPlayerUUID };
        const Player* oldPlayer { oldWorld->FindPlayer( playerUUID ) };

        if( !oldPlayer )
        {
          mCreatedPlayers.mCreatedPlayers.push_back( newPlayer );
          continue;
        }

        const NetVarRegistration& playerNetVars{ PlayerNetVarsGet() };
        const NetVarRegistration::DiffParams diffParams
        {
          .mOld{ oldPlayer },
          .mNew{ newPlayer },
        };
        const NetVarDiff bitfield{ playerNetVars.Diff( diffParams ) };
        if( bitfield.Empty() )
          continue;

        const ModifiedPlayer modifiedPlayers
        {
          .mPlayer     { newPlayer },
          .mDiff       { bitfield },
        };
        mModifiedPlayers.mModifiedPlayers.push_back( modifiedPlayers );
      }
    }

    ModifiedPlayers mModifiedPlayers;
    DeletedPlayers mDeletedPlayers;
    CreatedPlayers mCreatedPlayers;
  };


  // ----------------------------------------------------------------------------------------------

  void PlayerDiffs::Write( WorldsToDiff worldDiff, WriteStream* writer )
  {
    PlayerDiffsAux playerDiffs( worldDiff );
    playerDiffs.mDeletedPlayers.Write( writer );
    playerDiffs.mCreatedPlayers.Write( writer );
    playerDiffs.mModifiedPlayers.Write( writer );
  }

  void PlayerDiffs::Read( World* world, ReadStream* reader, Errors& errors )
  {
    TAC_CALL( DeletedPlayers::Read( world, reader, errors ) );
    TAC_CALL( CreatedPlayers::Read( world, reader, errors ) );
    TAC_CALL( ModifiedPlayers::Read( world, reader, errors ) );
  }

  // ----------------------------------------------------------------------------------------------


} // namespace Tac



