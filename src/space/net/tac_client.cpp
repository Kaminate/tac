#include "src/common/math/tac_vector2.h"
#include "src/common/math/tac_vector3.h"
#include "src/common/math/tac_vector4.h"
#include "src/common/memory/tac_memory.h"
#include "src/common/preprocess/tac_preprocessor.h"
#include "src/common/dataprocess/tac_serialization.h"
#include "src/common/string/tac_string_util.h"

#include "space/physics/collider/tac_collider.h"
#include "space/net/tac_client.h"
#include "space/ecs/tac_component.h"
#include "space/ecs/tac_entity.h"
#include "space/player/tac_player.h"

namespace Tac
{


  void ClientData::ReadEntityDifferences( Reader* reader,
                                          Errors& errors )
  {
    EntityUUID entityUUID;
    if( !reader->Read( &entityUUID ) )
      TAC_RAISE_ERROR( "failed to read different entity uuid");

    auto entity = mWorld->FindEntity( entityUUID );
    if( !entity )
      entity = mWorld->SpawnEntity( entityUUID );

    char deletedComponentsBitfield;
    if( !reader->Read( &deletedComponentsBitfield ) )
      TAC_RAISE_ERROR( "failed to read entity deleted component bits");

    TAC_ASSERT_UNIMPLEMENTED;
    //for( int iComponentType = 0; iComponentType < ( int )ComponentRegistryEntryIndex::Count; ++iComponentType )
    //{
    //  if( !( deletedComponentsBitfield & iComponentType ) )
    //    continue;
    //  auto componentType = ( ComponentRegistryEntryIndex )iComponentType;
    //  entity->RemoveComponent( componentType );
    //}

    char changedComponentsBitfield;
    if( !reader->Read( &changedComponentsBitfield ) )
      TAC_RAISE_ERROR( "failed to read entity changed component bitfield");
    TAC_ASSERT_UNIMPLEMENTED;
    //for( int iComponentType = 0; iComponentType < ( int )ComponentRegistryEntryIndex::Count; ++iComponentType )
    //{
    //  if( !( changedComponentsBitfield & iComponentType ) )
    //    continue;
    //  auto componentType = ( ComponentRegistryEntryIndex )iComponentType;
    //  auto component = entity->GetComponent( componentType );
    //  if( !component )
    //    component = entity->AddNewComponent( componentType );
    //  auto componentStuff = GetComponentData( componentType );

    //  component->PreReadDifferences();
    //  if( !reader->Read( component, componentStuff->mNetworkBits ) )
    //  {
    //    errors += "fuck";
    //    TAC_HANDLE_ERROR();
    //  }
    //  component->PostReadDifferences();
    //}
  }

  void ClientData::ReadPlayerDifferences( Reader* reader,
                                          Errors& errors )
  {
    PlayerUUID differentPlayerUUID;
    if( !reader->Read( &differentPlayerUUID ) )
      TAC_RAISE_ERROR( "failed to read player uuid");
    auto player = mWorld->FindPlayer( differentPlayerUUID );
    if( !player )
      player = mWorld->SpawnPlayer( differentPlayerUUID );
    if( !reader->Read( player, PlayerNetworkBitsGet()  ) )
      TAC_RAISE_ERROR( "failed to read player bits");
  }

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
    Timestamp newGameTime;
    if( !reader->Read( &newGameTime.mSeconds ) )
    {
      TAC_RAISE_ERROR( "failed to read new snapshot time");
    }

    if( newGameTime < mMostRecentSnapshotTime )
      return;
    mMostRecentSnapshotTime = newGameTime;


    Timestamp oldGameTime;
    if( !reader->Read( &oldGameTime.mSeconds ) )
    {
      TAC_RAISE_ERROR( "failed to read old snapshot time");
    }

    auto snapshotFrom = mSnapshots.FindSnapshot( oldGameTime );
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

    if( !reader->Read( ( UUID* )&mPlayerUUID ) )
    {
      TAC_RAISE_ERROR( "Failed to read player UUID");
    }

    PlayerCount numPlayerDeleted;
    if( !reader->Read( &numPlayerDeleted ) )
    {
      TAC_RAISE_ERROR( "Failed to read deleted player count");
    }
    for( PlayerCount i = 0; i < numPlayerDeleted; ++i )
    {
      PlayerUUID deletedPlayerUUID;
      if( !reader->Read( ( UUID* )&deletedPlayerUUID ) )
      {
        TAC_RAISE_ERROR( "Failed to read deleted player uuid");
      }

      mWorld->KillPlayer( deletedPlayerUUID );
    }

    PlayerCount numPlayersDifferent;
    if( !reader->Read( &numPlayersDifferent ) )
    {
      TAC_RAISE_ERROR( "Failed to read different player count");
    }
    for( PlayerCount i = 0; i < numPlayersDifferent; ++i )
    {
      TAC_CALL( ReadPlayerDifferences, reader, errors );
    }

    EntityCount numDeletedEntities;
    if( !reader->Read( &numDeletedEntities ) )
    {
      TAC_RAISE_ERROR( "Failed to read deleted entity count");
    }
    for( EntityCount i = 0; i < numDeletedEntities; ++i )
    {
      EntityUUID entityUUID;
      if( !reader->Read( ( UUID* )&entityUUID ) )
      {
        TAC_RAISE_ERROR( "Failed to read deleted entity uuid");
      }
      mWorld->KillEntity( entityUUID );
    }

    EntityCount numEntitiesDifferent;
    if( !reader->Read( &numEntitiesDifferent ) )
    {
      TAC_RAISE_ERROR( "Failed to read different entity count");
    }

    for( EntityCount i = 0; i < numEntitiesDifferent; ++i )
      ReadEntityDifferences( reader, errors );

    mSnapshots.AddSnapshot( mWorld );;

    if( mIsPredicting )
      ApplyPrediction( oldGameTime );
  }

  void ClientData::ExecuteNetMsg( void* bytes,
                                  int byteCount,
                                  Errors& errors )
  {
    Reader reader;
    reader.mFrom = GameEndianness;
    reader.mTo = GetEndianness();
    reader.mBegin = bytes;
    reader.mEnd = ( char* )bytes + byteCount;
    NetMsgType networkMessage = NetMsgType::Count;
    TAC_CALL( ReadNetMsgHeader, &reader, &networkMessage, errors );

    switch( networkMessage )
    {
      case NetMsgType::Snapshot:
      {
        TAC_CALL( ReadSnapshotBody, &reader, errors );
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
      TAC_CALL( ExecuteNetMsg, delayedMsg.data(), ( int )delayedMsg.size(), errors );
    }

    Writer writer;
    writer.mFrom = GetEndianness();
    writer.mTo = GameEndianness;

    WriteNetMsgHeader( &writer, NetMsgType::Input );

    SavedInput newInput;
    newInput.mTimestamp = mWorld->mElapsedSecs;
    newInput.mInputDirection = inputDir;
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

    sendNetworkMessageCallback(
      writer.mBytes.data(),
      ( int )writer.mBytes.size(),
      userData );

    mWorld->Step( seconds );

    //if( mChat.mIsReadyToSend )
    //{
    //  // todo: keep sending until the client has acknowledged our message
    //  mChat.mIsReadyToSend = false;
    //  writer->mBuffer = buffer;
    //  WriteOutgoingChatMessage( writer, &mChat, errors );
    //  TAC_HANDLE_ERROR();
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

}

