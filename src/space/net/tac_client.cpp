#include "src/common/math/tac_vector2.h"
#include "src/common/math/tac_vector3.h"
#include "src/common/math/tac_vector4.h"
#include "src/common/memory/tac_memory.h"
#include "src/common/preprocess/tac_preprocessor.h"
#include "src/common/dataprocess/tac_serialization.h"
#include "src/common/string/tac_string_util.h"

#include "space/physics/collider/tac_collider.h"
#include "space/net/tac_client.h"
#include "space/net/tac_player_diff.h"
#include "space/net/tac_entity_diff.h"
#include "space/ecs/tac_component.h"
#include "space/ecs/tac_entity.h"
#include "space/player/tac_player.h"

namespace Tac
{




  void ClientData::WriteInputBody( Writer* writer )
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

  void ClientData::ApplyPrediction( Timestamp lastTime )
  {
    Entity* entity = mWorld->FindEntity( mPlayerUUID );
    if( !entity )
      return;

    Player* player = mWorld->FindPlayer( mPlayerUUID );
    if( !player )
      return;

    for( const SavedInput& savedInput : mSavedInputs )
    {
      auto timeDifference = ( float )( savedInput.mTimestamp - lastTime );
      if( timeDifference < 0 )
        continue;

      lastTime = savedInput.mTimestamp;
      Collider* collider = Collider::GetCollider( entity );
      if( !collider )
        continue;

      player->mInputDirection = savedInput.mInputDirection;
      mWorld->ApplyInput( player, timeDifference );
      entity->mRelativeSpace.mPosition += collider->mVelocity * timeDifference;
      // TODO: this doesn't play nice with physics system integrate?
    }
  }


  void ClientData::ReadSnapshotBody( Reader* reader,
                                     Errors& errors )
  {
    const auto newGameTime = TAC_CALL( reader->Read< Timestamp >( errors ) );
    if( newGameTime < mMostRecentSnapshotTime )
      return;

    mMostRecentSnapshotTime = newGameTime;

    const auto oldGameTime = TAC_CALL( reader->Read<Timestamp >( errors ) );

    World* snapshotFrom = mSnapshots.FindSnapshot( oldGameTime );
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

    TAC_CALL( mPlayerUUID = reader->Read<PlayerUUID>( errors ) );

    TAC_CALL( PlayerDiffs::Read(mWorld, reader, errors ) );
    TAC_CALL( EntityDiffs::Read( mWorld, reader, errors ) );

    mSnapshots.AddSnapshot( mWorld );;

    if( mIsPredicting )
      ApplyPrediction( oldGameTime );
  }

  void ClientData::ExecuteNetMsg( void* bytes,
                                  int byteCount,
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
      case NetMsgType::Snapshot:
      {
        TAC_CALL( ReadSnapshotBody( &reader, errors ) );
      } break;
      //case NetMsgType::Text: { ReadIncomingChatMessageBody( &reader, &mChat, errors ); } break;
    }
  }


  void ClientData::Update( float seconds,
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

    Writer writer
    {
      .mFrom = GetEndianness(),
      .mTo = GameEndianness,
    };

    WriteNetMsgHeader( &writer, NetMsgType::Input );

    SavedInput newInput
    {
      .mTimestamp = mWorld->mElapsedSecs,
      .mInputDirection = inputDir,
    };

    if( mSavedInputs.size() == ClientData::sMaxSavedInputCount )
      mSavedInputs.pop_front();
    mSavedInputs.push_back( newInput );

    if( mIsPredicting )
    {
      auto player = mWorld->FindPlayer( mPlayerUUID );
      if( player )
        player->mInputDirection = inputDir;
    }

    WriteInputBody( &writer );

    sendNetworkMessageCallback( writer.mBytes.data(),
                                ( int )writer.mBytes.size(),
                                userData );

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

  void ClientData::ReceiveMessage( void* bytes,
                                   int byteCount,
                                   Errors& errors )
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

