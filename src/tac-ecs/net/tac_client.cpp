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
    Entity* entity { mWorld->FindEntity( mPlayerUUID ) };
    if( !entity )
      return;

    Player* player { mWorld->FindPlayer( mPlayerUUID ) };
    if( !player )
      return;

    for( const SavedInput& savedInput : mSavedInputs )
    {
      auto timeDifference { ( float )( savedInput.mTimestamp - lastTime ) };
      if( timeDifference < 0 )
        continue;

      lastTime = savedInput.mTimestamp;
      Collider* collider { Collider::GetCollider( entity ) };
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
    TAC_CALL( const Timestamp newGameTime{ reader->Read< Timestamp >( errors ) } );
    if( newGameTime < mMostRecentSnapshotTime )
      return;

    mMostRecentSnapshotTime = newGameTime;

    TAC_CALL( const Timestamp oldGameTime{ reader->Read<Timestamp >( errors ) } );

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
      .mFrom  { GameEndianness },
      .mTo    { GetEndianness() },
      .mBegin { bytes },
      .mEnd   { ( char* )bytes + byteCount },
    };

    TAC_CALL( const NetMsgType networkMessage{ ReadNetMsgHeader( &reader, errors ) } );

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
      .mFrom { GetEndianness() },
      .mTo   { GameEndianness },
    };

    WriteNetMsgHeader( &writer, NetMsgType::Input );

    SavedInput newInput
    {
      .mTimestamp { mWorld->mElapsedSecs },
      .mInputDirection { inputDir },
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

