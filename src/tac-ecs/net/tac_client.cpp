#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/dataprocess/tac_serialization.h"
#include "tac-std-lib/string/tac_string_util.h"

#include "tac-ecs/physics/collider/tac_collider.h"
#include "tac-ecs/net/tac_client.h"
#include "tac-ecs/net/tac_player_diff.h"
#include "tac-ecs/net/tac_entity_diff.h"
#include "tac-ecs/component/tac_component.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/player/tac_player.h"

namespace Tac
{
  static const int sMaxSavedInputCount { 60 };

  void ClientData::WriteInputBody( WriteStream* writer )
  {
    writer->Write( mSavedInputs.back().mInputDirection );
    writer->Write( mMostRecentSnapshotTime );
  }

  void ClientData::OnClientDisconnect()
  {
    mWorld->mEntities.clear();
    mWorld->mPlayers.clear();
    mPlayerUUID = ( PlayerUUID )0;
  }

  void ClientData::ApplyPrediction( GameTime lastTime )
  {
    Entity* entity { mWorld->FindEntity( mPlayerUUID ) };
    if( !entity )
      return;

    Player* player { mWorld->FindPlayer( mPlayerUUID ) };
    if( !player )
      return;

    for( const SavedInput& savedInput : mSavedInputs )
    {
      TimeDelta timeDifference { savedInput.mGameTime - lastTime };
      if( timeDifference < 0 )
        continue;

      lastTime = savedInput.mGameTime;
      Collider* collider { Collider::GetCollider( entity ) };
      if( !collider )
        continue;

      player->mInputDirection = savedInput.mInputDirection;
      mWorld->ApplyInput( player, timeDifference );
      entity->mRelativeSpace.mPosition += collider->mVelocity * timeDifference;
      // TODO: this doesn't play nice with physics system integrate?
    }
  }

  void ClientData::ReadSnapshotBody( ReadStream* reader, Errors& errors )
  {
    TAC_CALL( const GameTime newGameTime{ reader->Read< GameTime >( errors ) } );
    if( newGameTime < mMostRecentSnapshotTime )
      return;

    mMostRecentSnapshotTime = newGameTime;

    TAC_CALL( const GameTime oldGameTime{ reader->Read<GameTime >( errors ) } );

    World* snapshotFrom { mSnapshots.FindSnapshot( oldGameTime ) };
    if( !snapshotFrom )
    {
      if( oldGameTime ) // we need the server to send us the full state
        return;
      snapshotFrom = mEmptyWorld;
    }

    // Ok so the problem is
    // Well theres several problems
    //
    // Might also be problems that we havent solved yet.
    //
    // ANyway.
    //
    // Problem: when we deep copy from the snapshot, we destroy our entities, so any previous databreakpoint is invalidated
    //TODO;

    mWorld->DeepCopy( *snapshotFrom );
    mWorld->mElapsedSecs = newGameTime;

    TAC_CALL( mPlayerUUID = reader->Read< PlayerUUID >( errors ) );
    TAC_CALL( PlayerDiffs::Read( mWorld, reader, errors ) );
    TAC_CALL( EntityDiffAPI::Read( mWorld, reader, errors ) );

    mSnapshots.AddSnapshot( mWorld );;

    if( mIsPredicting )
      ApplyPrediction( oldGameTime );
  }

  void ClientData::ExecuteNetMsg( const void* bytes, int byteCount, Errors& errors )
  {
    ReadStream readStream{ .mBytes { ( const char* )bytes, byteCount }, };

    TAC_CALL( const NetMsgType networkMessage{ ReadNetMsgHeader( &readStream, errors ) } );

    switch( networkMessage )
    {
      case NetMsgType::Snapshot:
      {
        TAC_CALL( ReadSnapshotBody( &readStream, errors ) );
      } break;
      //case NetMsgType::Text: { ReadIncomingChatMessageBody( &reader, &mChat, errors ); } break;
    }
  }

  void ClientData::Update( TimeDelta seconds,
                           v2 inputDir,
                           ClientSendNetworkMessageCallback sendNetworkMessageCallback,
                           void* userData,
                           Errors& errors )
  {
    Vector< char > delayedMsg;
    while( mSavedNetworkMessages.TryPopSavedMessage( delayedMsg, mWorld->mElapsedSecs ) )
    {
      TAC_CALL( ExecuteNetMsg( delayedMsg.data(), ( int )delayedMsg.size(), errors ) );
    }

    WriteStream writer{};
    WriteNetMsgHeader( &writer, NetMsgType::Input );

    const SavedInput newInput
    {
      .mGameTime      { mWorld->mElapsedSecs },
      .mInputDirection { inputDir },
    };

    if( mSavedInputs.size() == sMaxSavedInputCount )
      mSavedInputs.pop_front();
    mSavedInputs.push_back( newInput );

    if( mIsPredicting )
      if( Player* player { mWorld->FindPlayer( mPlayerUUID ) } )
        player->mInputDirection = inputDir;

    WriteInputBody( &writer );
    sendNetworkMessageCallback( writer.Data(), writer.Size(), userData );
    mWorld->Step( seconds );

    //if( mChat.mIsReadyToSend )
    //{
    //  // todo: keep sending until the client has acknowledged our message
    //  mChat.mIsReadyToSend = false;
    //  writer->mBuffer = buffer;
    //  TAC_CALL( WriteOutgoingChatMessage( writer, &mChat, errors ) );
    //  sendNetworkMessageCallback(
    //    ( u8* )writer->mBuffer.mBytes,
    //    writer->mBuffer.mByteCountCur,
    //    userData );
    //}
  }

  void ClientData::ReceiveMessage( void* bytes, int byteCount, Errors& errors )
  {
    if( mSavedNetworkMessages.mLagSimulationMS )
    {
      Vector< char > messageData( ( char* )bytes, ( char* )bytes + byteCount );
      mSavedNetworkMessages.SaveMessage( messageData, mWorld->mElapsedSecs );
      return;
    }
    ExecuteNetMsg( bytes, byteCount, errors );
  }

} // namespace Tac

