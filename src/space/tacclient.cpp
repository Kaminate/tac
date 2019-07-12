#include "space/tacclient.h"
#include "space/tacworld.h"
#include "space/tacentity.h"
#include "space/tacplayer.h"
#include "space/taccomponent.h"
#include "space/tacspacenet.h"
#include "space/tacspacenet.h"
#include "space/collider/taccollider.h"
#include "common/tacSerialization.h"
#include "common/tacUtility.h"
#include "common/tacPreprocessor.h"
#include "common/tacMemory.h"
#include "common/math/tacVector2.h"
#include "common/math/tacVector3.h"
#include "common/math/tacVector4.h"



void TacClientData::ReadEntityDifferences(
  TacReader* reader,
  TacErrors& errors )
{
  TacEntityUUID entityUUID;
  if( !reader->Read( &entityUUID ) )
  {
    errors += "fuck";
    TAC_HANDLE_ERROR( errors );
  }

  auto entity = mWorld->FindEntity( entityUUID );
  if( !entity )
    entity = mWorld->SpawnEntity( entityUUID );

  char deletedComponentsBitfield;
  if( !reader->Read( &deletedComponentsBitfield ) )
  {
    errors += "fuck";
    TAC_HANDLE_ERROR( errors );
  }

  TacUnimplemented;
  //for( int iComponentType = 0; iComponentType < ( int )TacComponentRegistryEntryIndex::Count; ++iComponentType )
  //{
  //  if( !( deletedComponentsBitfield & iComponentType ) )
  //    continue;
  //  auto componentType = ( TacComponentRegistryEntryIndex )iComponentType;
  //  entity->RemoveComponent( componentType );
  //}

  char changedComponentsBitfield;
  if( !reader->Read( &changedComponentsBitfield ) )
  {
    errors += "fuck";
    TAC_HANDLE_ERROR( errors );
  }
  TacUnimplemented;
  //for( int iComponentType = 0; iComponentType < ( int )TacComponentRegistryEntryIndex::Count; ++iComponentType )
  //{
  //  if( !( changedComponentsBitfield & iComponentType ) )
  //    continue;
  //  auto componentType = ( TacComponentRegistryEntryIndex )iComponentType;
  //  auto component = entity->GetComponent( componentType );
  //  if( !component )
  //    component = entity->AddNewComponent( componentType );
  //  auto componentStuff = TacGetComponentData( componentType );

  //  component->PreReadDifferences();
  //  if( !reader->Read( component, componentStuff->mNetworkBits ) )
  //  {
  //    errors += "fuck";
  //    TAC_HANDLE_ERROR( errors );
  //  }
  //  component->PostReadDifferences();
  //}
}

void TacClientData::ReadPlayerDifferences(
  TacReader* reader,
  TacErrors& errors )
{
  TacPlayerUUID differentPlayerUUID;
  if( !reader->Read( &differentPlayerUUID ) )
  {
    errors += "fuck";
    TAC_HANDLE_ERROR( errors );
  }
  auto player = mWorld->FindPlayer( differentPlayerUUID );
  if( !player )
    player = mWorld->SpawnPlayer( differentPlayerUUID );
  if( !reader->Read( player, TacPlayerBits ) )
  {
    errors += "fuck";
    TAC_HANDLE_ERROR( errors );
  }
}

void TacClientData::WriteInputBody( TacWriter* writer )
{
  writer->Write( mSavedInputs.back().mInputDirection );
  writer->Write( mMostRecentSnapshotTime );
}

void TacClientData::TacOnClientDisconnect()
{
  mWorld->mEntities.clear();
  mWorld->mPlayers.clear();
  mPlayerUUID = ( TacPlayerUUID )0;
}

void TacClientData::ApplyPrediction( double lastTime )
{
  auto entity = mWorld->FindEntity( mPlayerUUID );
  if( !entity )
    return;
  auto player = mWorld->FindPlayer( mPlayerUUID );
  if( !player )
    return;
  for( auto savedInput : mSavedInputs )
  {
    auto timeDifference = ( float )( savedInput.mTimestamp - lastTime );
    if( timeDifference < 0 )
      continue;
    lastTime = savedInput.mTimestamp;
    TacCollider* collider = TacCollider::GetCollider( entity );
    if( !collider )
      continue;
    player->mInputDirection = savedInput.mInputDirection;
    mWorld->ApplyInput( player, timeDifference );
    entity->mLocalPosition += collider->mVelocity * timeDifference;
    // TODO: this doesn't play nice with physics system integrate?
  }
}


void TacClientData::TacReadSnapshotBody(
  TacReader* reader,
  TacErrors& errors )
{
  double newGameTime;
  if( !reader->Read( &newGameTime ) )
  {
    errors += "Fuck";
    return;
  }

  if( newGameTime < mMostRecentSnapshotTime )
    return;
  mMostRecentSnapshotTime = newGameTime;


  double oldGameTime;
  if( !reader->Read( &oldGameTime ) )
  {
    errors += "Fuck";
    return;
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
  //TacTODO;

  mWorld->DeepCopy( *snapshotFrom );
  mWorld->mElapsedSecs = newGameTime;

  if( !reader->Read( ( TacUUID* )&mPlayerUUID ) )
  {
    errors += "Fuck";
    return;
  }

  TacPlayerCount numPlayerDeleted;
  if( !reader->Read( &numPlayerDeleted ) )
  {
    errors += "Fuck";
    return;
  }
  for( TacPlayerCount i = 0; i < numPlayerDeleted; ++i )
  {
    TacPlayerUUID deletedPlayerUUID;
    if( !reader->Read( ( TacUUID* )&deletedPlayerUUID ) )
    {
      errors += "Fuck";
      return;
    }

    mWorld->KillPlayer( deletedPlayerUUID );
  }

  TacPlayerCount numPlayersDifferent;
  if( !reader->Read( &numPlayersDifferent ) )
  {
    errors += "Fuck";
    return;
  }
  for( TacPlayerCount i = 0; i < numPlayersDifferent; ++i )
  {
    ReadPlayerDifferences( reader, errors );
    TAC_HANDLE_ERROR( errors );
  }

  TacEntityCount numDeletedEntities;
  if( !reader->Read( &numDeletedEntities ) )
  {
    errors += "Fuck";
    return;
  }
  for( TacEntityCount i = 0; i < numDeletedEntities; ++i )
  {
    TacEntityUUID entityUUID;
    if( !reader->Read( ( TacUUID* )&entityUUID ) )
    {
      errors += "fuck";
      return;
    }
    mWorld->KillEntity( entityUUID );
  }

  TacEntityCount numEntitiesDifferent;
  if( !reader->Read( &numEntitiesDifferent ) )
  {
    errors += "Fuck";
    return;
  }

  for( TacEntityCount i = 0; i < numEntitiesDifferent; ++i )
    ReadEntityDifferences( reader, errors );

  mSnapshots.AddSnapshot( mWorld );;

  if( mIsPredicting )
    ApplyPrediction( oldGameTime );
}

void TacClientData::ExecuteNetMsg(
  void* bytes,
  int byteCount,
  TacErrors& errors )
{
  TacReader reader;
  reader.mFrom = TacGameEndianness;
  reader.mTo = TacGetEndianness();
  reader.mBegin = bytes;
  reader.mEnd = ( char* )bytes + byteCount;
  auto networkMessage = TacReadNetMsgHeader( &reader, errors );
  TAC_HANDLE_ERROR( errors );

  switch( networkMessage )
  {
    case TacNetMsgType::Snapshot:
    {
      TacReadSnapshotBody( &reader, errors );
      TAC_HANDLE_ERROR( errors );
    } break;
      //case TacNetMsgType::Text: { TacReadIncomingChatMessageBody( &reader, &mChat, errors ); } break;
  }
}


void TacClientData::Update(
  float seconds,
  v2 inputDir,
  ClientSendNetworkMessageCallback sendNetworkMessageCallback,
  void* userData,
  TacErrors& errors )
{
  TacVector< char > delayedMsg;
  while( mSavedNetworkMessages.TryPopSavedMessage( delayedMsg, mWorld->mElapsedSecs ) )
  {
    ExecuteNetMsg( delayedMsg.data(), ( int )delayedMsg.size(), errors );
    TAC_HANDLE_ERROR( errors );
  }

  TacWriter writer;
  writer.mFrom = TacGetEndianness();
  writer.mTo = TacGameEndianness;
  TacWriteNetMsgHeader( &writer, TacNetMsgType::Input );
  TAC_HANDLE_ERROR( errors );

  TacSavedInput newInput;
  newInput.mTimestamp = mWorld->mElapsedSecs;
  newInput.mInputDirection = inputDir;
  if( mSavedInputs.size() == TacClientData::sMaxSavedInputCount )
    mSavedInputs.pop_front();
  mSavedInputs.push_back( newInput );

  if( mIsPredicting )
  {
    auto player = mWorld->FindPlayer( mPlayerUUID );
    if( player )
      player->mInputDirection = inputDir;
  }

  WriteInputBody( &writer );
  TAC_HANDLE_ERROR( errors );

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
  //  TacWriteOutgoingChatMessage( writer, &mChat, errors );
  //  TAC_HANDLE_ERROR( errors );
  //  sendNetworkMessageCallback(
  //    ( u8* )writer->mBuffer.mBytes,
  //    writer->mBuffer.mByteCountCur,
  //    userData );
  //}
}

void TacClientData::ReceiveMessage(
  void* bytes,
  int byteCount,
  TacErrors& errors )
{
  if( mSavedNetworkMessages.mLagSimulationMS )
  {
    mSavedNetworkMessages.SaveMessage( TacTemporaryMemory( bytes, byteCount ), mWorld->mElapsedSecs );
    return;
  }
  ExecuteNetMsg( bytes, byteCount, errors );
}
