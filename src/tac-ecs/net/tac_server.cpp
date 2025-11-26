#include "tac_server.h" // self-inc

#include "tac-ecs/component/tac_component.h"
#include "tac-ecs/component/tac_component_registry.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/net/tac_entity_diff.h"
#include "tac-ecs/net/tac_player_diff.h"
#include "tac-ecs/net/tac_space_net.h"
#include "tac-ecs/player/tac_player.h"
#include "tac-ecs/script/tac_script.h"
#include "tac-ecs/scripts/tac_script_game_client.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-std-lib/containers/tac_set.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string_util.h"

namespace Tac
{
  // decreasing to 0 for easier debugging
  // ( consistant update between server/client )
  static const GameTimeDelta sSnapshotUntilNextSecondsMax {}; //0.1f;

  ServerData::ServerData()
  {
    mWorld = TAC_NEW World();
    mEmptyWorld = TAC_NEW World();
  }

  ServerData::~ServerData()
  {
    for( OtherPlayer* otherPlayer : mOtherPlayers )
      TAC_DELETE otherPlayer;
    TAC_DELETE mWorld;
    TAC_DELETE mEmptyWorld;
  }

  void ServerData::DebugImgui()
  {
    if( mWorld )
    {
      mWorld->DebugImgui();
    }
  }

  void ServerData::OnClientJoin( ConnectionUUID connectionID )
  {
    Player* player { SpawnPlayer() };
    TAC_ASSERT( player );
    TAC_ASSERT( mOtherPlayers.size() < ServerData::sOtherPlayerCountMax );
    OtherPlayer* otherPlayer { TAC_NEW OtherPlayer() };
    otherPlayer->mPlayerUUID = player->mPlayerUUID;
    otherPlayer->mConnectionUUID = connectionID;
    mOtherPlayers.push_back( otherPlayer );
  }


  auto ServerData::FindOtherPlayer( ConnectionUUID connectionID ) -> OtherPlayer*
  {
    for( OtherPlayer* otherPlayer : mOtherPlayers )
      if( otherPlayer->mConnectionUUID == connectionID )
        return otherPlayer;
    return nullptr;
  }

  void ServerData::OnLoseClient( ConnectionUUID connectionID )
  {
    auto it = [ & ]()
      {
        for( auto it { mOtherPlayers.begin() }; it != mOtherPlayers.end(); ++it )
          if( OtherPlayer* otherPlayer { *it };otherPlayer->mConnectionUUID == connectionID )
            return it;
        return mOtherPlayers.end();
      }( );

    if( it == mOtherPlayers.end() )
      return;

    OtherPlayer* otherPlayer { *it };
    mWorld->KillPlayer( otherPlayer->mPlayerUUID );
    TAC_DELETE otherPlayer;
    mOtherPlayers.erase( it );
  }

  void ServerData::ReadInput( ReadStream* reader,
                              ConnectionUUID connectionID,
                              Errors& errors )
  {
    if( OtherPlayer * otherPlayer{ TAC_VERIFY( FindOtherPlayer( connectionID ) ) } )
    {
      if( Player * player{ mWorld->FindPlayer( otherPlayer->mPlayerUUID ) } )
      {
        TAC_CALL( player->mInputDirection = reader->Read<v2>( errors ) );
        TAC_CALL( otherPlayer->mGameTime = reader->Read<GameTime>( errors ) );
      }
    }
  }

  // OtherPlayer is the player who is the receipient of this snapshot
  void ServerData::WriteSnapshotBody( OtherPlayer* otherPlayer, WriteStream* writer )
  {
    writer->Write( mWorld->mElapsedSecs );
    TAC_ASSERT( otherPlayer );
    World* oldWorld { mSnapshots.FindSnapshot( otherPlayer->mGameTime ) };
    if( !oldWorld )
      oldWorld = mEmptyWorld;

    writer->Write( otherPlayer->mGameTime );
    writer->Write( ( UUID )otherPlayer->mPlayerUUID );
    const WorldsToDiff worldDiff
    {
      .mOldWorld{ oldWorld },
      .mNewWorld{ mWorld },
    };
    PlayerDiffs::Write( worldDiff, writer );
    EntityDiffAPI::Write( worldDiff, writer );
  }

  void ServerData::ExecuteNetMsg( const ConnectionUUID connectionID,
                                  const void* bytes,
                                  const int byteCount,
                                  Errors& errors )
  {
    ReadStream reader { .mBytes { ( const char* )bytes, byteCount }, };

    TAC_CALL( const auto networkMessage { ReadNetMsgHeader( &reader, errors )  });
    switch( networkMessage )
    {
      case NetMsgType::Input:
        ReadInput( &reader, connectionID, errors );
        break;
        //case NetMsgType::Text:
        //{
        //  ReadIncomingChatMessageBody(
        //    &reader,
        //    &mChat,
        //    errors );
        //} break;
    }
  }

  auto ServerData::SpawnEntity() -> Entity*
  {
    return mWorld->SpawnEntity( mEntityUUIDCounter.AllocateNewUUID() );
  }

  auto ServerData::SpawnPlayer() -> Player*
  {
    return mWorld->mPlayers.size() < sPlayerCountMax
      ? mWorld->SpawnPlayer( mPlayerUUIDCounter.AllocateNewUUID() )
      : nullptr;
  }


  void ServerData::ReceiveMessage( ConnectionUUID connectionID,
                                   void* bytes,
                                   int byteCount,
                                   Errors& errors )
  {
    OtherPlayer* otherPlayer { FindOtherPlayer( connectionID ) };
    if( !otherPlayer )
      return;

    if( otherPlayer->delayedNetMsg.mLagSimulationMS )
    {
      Vector< char > messageData;
      messageData.resize( byteCount );
      MemCpy( messageData.data(), bytes, byteCount );

      otherPlayer->delayedNetMsg.SaveMessage( messageData, mWorld->mElapsedSecs );
      return;
    }

    ExecuteNetMsg(
      connectionID,
      bytes,
      byteCount,
      errors );
  }

  void ServerData::Update( const GameTimeDelta seconds,
                           const ServerSendNetworkMessageCallback sendNetworkMessageCallback,
                           void* userData,
                           Errors& errors )
  {
    for( OtherPlayer* otherPlayer : mOtherPlayers )
    {
      Vector< char > savedNetMsg;
      while( otherPlayer->delayedNetMsg.TryPopSavedMessage( savedNetMsg, mWorld->mElapsedSecs ) )
      {
        const void* bytes { savedNetMsg.data() };
        const int byteCount { ( int )savedNetMsg.size() };
        TAC_CALL( ExecuteNetMsg( otherPlayer->mConnectionUUID, bytes, byteCount, errors ) );
      }
    }

    mWorld->Step( seconds );

    mSnapshotUntilNextSecondsCur -= seconds;
    if( mSnapshotUntilNextSecondsCur > 0 )
      return;

    mSnapshotUntilNextSecondsCur = sSnapshotUntilNextSecondsMax;
    mSnapshots.AddSnapshot( mWorld );

    for( OtherPlayer* otherPlayer : mOtherPlayers )
    {
      WriteStream writer{};
      WriteNetMsgHeader( &writer, NetMsgType::Snapshot );
      WriteSnapshotBody( otherPlayer, &writer );
      if( sendNetworkMessageCallback )
      {
        const void* bytes { writer.Data() };
        const int byteCount { ( int )writer.Size() };
        sendNetworkMessageCallback( otherPlayer->mConnectionUUID,
                                    bytes,
                                    byteCount,
                                    userData );
      }
    }

    //if( mChat.mIsReadyToSend )
    //{
    //  // todo: keep sending until the client has acknowled our message
    //  mChat.mIsReadyToSend = false;
    //  writer->mBuffer = buffer;
    //  TAC_CALL( WriteOutgoingChatMessage( writer, &mChat, errors ) );
    //  for( OtherPlayer* otherPlayer : mOtherPlayers )
    //  {
    //    sendNetworkMessageCallback(
    //      otherPlayer->mConnectionUUID,
    //      ( u8* )writer->mBuffer.mBytes,
    //      writer->mBuffer.mByteCountCur,
    //      userData );
    //  }
    //}
  }


}

