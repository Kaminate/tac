#include "space/net/tac_server.h" // self-inc

#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/dataprocess/tac_settings.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/containers/tac_set.h"
#include "space/ecs/tac_component.h"
#include "space/ecs/tac_entity.h"
#include "space/ecs/tac_component_registry.h"
#include "space/player/tac_player.h"
#include "space/script/tac_script.h"
#include "space/scripts/tac_script_game_client.h"
#include "space/net/tac_space_net.h"
#include "space/net/tac_entity_diff.h"
#include "space/net/tac_player_diff.h"
#include "space/net/tac_space_net.h"
#include "space/world/tac_world.h"

namespace Tac
{
  // decreasing to 0 for easier debugging
  // ( consistant update between server/client )
  const float sSnapshotUntilNextSecondsMax = 0; //0.1f;

  ServerData::ServerData()
  {
    mWorld = new World();
    mEmptyWorld = new World();
  }

  ServerData::~ServerData()
  {
    for( OtherPlayer* otherPlayer : mOtherPlayers )
      delete otherPlayer;
    delete mWorld;
    delete mEmptyWorld;
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
    Player* player = SpawnPlayer();
    TAC_ASSERT( player );
    TAC_ASSERT( mOtherPlayers.size() < ServerData::sOtherPlayerCountMax );
    OtherPlayer* otherPlayer = new OtherPlayer();
    otherPlayer->mPlayerUUID = player->mPlayerUUID;
    otherPlayer->mConnectionUUID = connectionID;
    mOtherPlayers.push_back( otherPlayer );
  }


  OtherPlayer* ServerData::FindOtherPlayer( ConnectionUUID connectionID )
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
        for( auto it = mOtherPlayers.begin(); it != mOtherPlayers.end(); ++it )
        {
          OtherPlayer* otherPlayer = *it;
          if( otherPlayer->mConnectionUUID == connectionID )
            return it;
        }
        return mOtherPlayers.end();
      }( );

    if( it == mOtherPlayers.end() )
      return;

    OtherPlayer* otherPlayer = *it;
    mWorld->KillPlayer( otherPlayer->mPlayerUUID );
    delete otherPlayer;
    mOtherPlayers.erase( it );
  }

  void ServerData::ReadInput( Reader* reader,
                              ConnectionUUID connectionID,
                              Errors& errors )
  {
    OtherPlayer* otherPlayer = FindOtherPlayer( connectionID );
    TAC_ASSERT( otherPlayer );

    Player* player = mWorld->FindPlayer( otherPlayer->mPlayerUUID );
    if( !player )
      return;

    TAC_RAISE_ERROR_IF( !reader->Read( &player->mInputDirection ),
                        "failed to read player input direction" );

    TAC_RAISE_ERROR_IF( !reader->Read( &otherPlayer->mTimeStamp ),
                        "failed to read player time stamp" );
  }

  // OtherPlayer is the player who is the receipient of this snapshot
  void ServerData::WriteSnapshotBody( OtherPlayer* otherPlayer, Writer* writer )
  {
    writer->Write( mWorld->mElapsedSecs );
    TAC_ASSERT( otherPlayer );
    World* oldWorld = mSnapshots.FindSnapshot( otherPlayer->mTimeStamp );
    if( !oldWorld )
      oldWorld = mEmptyWorld;

    writer->Write( otherPlayer->mTimeStamp );
    writer->Write( ( UUID )otherPlayer->mPlayerUUID );

    PlayerDiffs::Write( oldWorld, mWorld, writer );
    EntityDiffs::Write( oldWorld, mWorld, writer );
  }

  void ServerData::ExecuteNetMsg( const ConnectionUUID connectionID,
                                  const void* bytes,
                                  const int byteCount,
                                  Errors& errors )
  {
    Reader reader
    {
      .mFrom = GameEndianness,
      .mTo = GetEndianness(),
      .mBegin = bytes,
      .mEnd = ( char* )bytes + byteCount,
    };

    const auto networkMessage = TAC_CALL( ReadNetMsgHeader( &reader, errors ) );
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

  Entity* ServerData::SpawnEntity()
  {
    return mWorld->SpawnEntity( mEntityUUIDCounter.AllocateNewUUID() );
  }

  Player* ServerData::SpawnPlayer()
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
    OtherPlayer* otherPlayer = FindOtherPlayer( connectionID );
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

  void ServerData::Update( const float seconds,
                           const ServerSendNetworkMessageCallback sendNetworkMessageCallback,
                           void* userData,
                           Errors& errors )
  {
    for( OtherPlayer* otherPlayer : mOtherPlayers )
    {
      Vector< char > savedNetMsg;
      while( otherPlayer->delayedNetMsg.TryPopSavedMessage( savedNetMsg, mWorld->mElapsedSecs ) )
      {
        const void* bytes = savedNetMsg.data();
        const int byteCount = ( int )savedNetMsg.size();
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
      Writer writer;
      writer.mFrom = GetEndianness();
      writer.mTo = GameEndianness;

      WriteNetMsgHeader( &writer, NetMsgType::Snapshot );
      WriteSnapshotBody( otherPlayer, &writer );
      if( sendNetworkMessageCallback )
      {
        const void* bytes = writer.mBytes.data();
        const int byteCount = ( int )writer.mBytes.size();
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

