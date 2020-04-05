
#include "src/space/tacClient.h"
#include "src/space/tacWorld.h"
#include "src/space/tacEntity.h"
#include "src/space/tacPlayer.h"
#include "src/space/tacComponent.h"
#include "src/space/tacSpacenet.h"
#include "src/space/tacSpacenet.h"
#include "src/space/collider/tacCollider.h"
#include "src/common/tacSerialization.h"
#include "src/common/tacUtility.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacMemory.h"
#include "src/common/math/tacVector2.h"
#include "src/common/math/tacVector3.h"
#include "src/common/math/tacVector4.h"

namespace Tac
{


void ClientData::ReadEntityDifferences(
  Reader* reader,
  Errors& errors )
{
  EntityUUID entityUUID;
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

  TAC_UNIMPLEMENTED;
  //for( int iComponentType = 0; iComponentType < ( int )ComponentRegistryEntryIndex::Count; ++iComponentType )
  //{
  //  if( !( deletedComponentsBitfield & iComponentType ) )
  //    continue;
  //  auto componentType = ( ComponentRegistryEntryIndex )iComponentType;
  //  entity->RemoveComponent( componentType );
  //}

  char changedComponentsBitfield;
  if( !reader->Read( &changedComponentsBitfield ) )
  {
    errors += "fuck";
    TAC_HANDLE_ERROR( errors );
  }
  TAC_UNIMPLEMENTED;
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
  //    TAC_HANDLE_ERROR( errors );
  //  }
  //  component->PostReadDifferences();
  //}
}

void ClientData::ReadPlayerDifferences(
  Reader* reader,
  Errors& errors )
{
  PlayerUUID differentPlayerUUID;
  if( !reader->Read( &differentPlayerUUID ) )
  {
    errors += "fuck";
    TAC_HANDLE_ERROR( errors );
  }
  auto player = mWorld->FindPlayer( differentPlayerUUID );
  if( !player )
    player = mWorld->SpawnPlayer( differentPlayerUUID );
  if( !reader->Read( player, PlayerBits ) )
  {
    errors += "fuck";
    TAC_HANDLE_ERROR( errors );
  }
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

void ClientData::ApplyPrediction( double lastTime )
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
    Collider* collider = Collider::GetCollider( entity );
    if( !collider )
      continue;
    player->mInputDirection = savedInput.mInputDirection;
    mWorld->ApplyInput( player, timeDifference );
    entity->mRelativeSpace.mPosition += collider->mVelocity * timeDifference;
    // TODO: this doesn't play nice with physics system integrate?
  }
}


void ClientData::ReadSnapshotBody(
  Reader* reader,
  Errors& errors )
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
  //TODO;

  mWorld->DeepCopy( *snapshotFrom );
  mWorld->mElapsedSecs = newGameTime;

  if( !reader->Read( ( UUID* )&mPlayerUUID ) )
  {
    errors += "Fuck";
    return;
  }

  PlayerCount numPlayerDeleted;
  if( !reader->Read( &numPlayerDeleted ) )
  {
    errors += "Fuck";
    return;
  }
  for( PlayerCount i = 0; i < numPlayerDeleted; ++i )
  {
    PlayerUUID deletedPlayerUUID;
    if( !reader->Read( ( UUID* )&deletedPlayerUUID ) )
    {
      errors += "Fuck";
      return;
    }

    mWorld->KillPlayer( deletedPlayerUUID );
  }

  PlayerCount numPlayersDifferent;
  if( !reader->Read( &numPlayersDifferent ) )
  {
    errors += "Fuck";
    return;
  }
  for( PlayerCount i = 0; i < numPlayersDifferent; ++i )
  {
    ReadPlayerDifferences( reader, errors );
    TAC_HANDLE_ERROR( errors );
  }

  EntityCount numDeletedEntities;
  if( !reader->Read( &numDeletedEntities ) )
  {
    errors += "Fuck";
    return;
  }
  for( EntityCount i = 0; i < numDeletedEntities; ++i )
  {
    EntityUUID entityUUID;
    if( !reader->Read( ( UUID* )&entityUUID ) )
    {
      errors += "fuck";
      return;
    }
    mWorld->KillEntity( entityUUID );
  }

  EntityCount numEntitiesDifferent;
  if( !reader->Read( &numEntitiesDifferent ) )
  {
    errors += "Fuck";
    return;
  }

  for( EntityCount i = 0; i < numEntitiesDifferent; ++i )
    ReadEntityDifferences( reader, errors );

  mSnapshots.AddSnapshot( mWorld );;

  if( mIsPredicting )
    ApplyPrediction( oldGameTime );
}

void ClientData::ExecuteNetMsg(
  void* bytes,
  int byteCount,
  Errors& errors )
{
  Reader reader;
  reader.mFrom = GameEndianness;
  reader.mTo = GetEndianness();
  reader.mBegin = bytes;
  reader.mEnd = ( char* )bytes + byteCount;
  auto networkMessage = ReadNetMsgHeader( &reader, errors );
  TAC_HANDLE_ERROR( errors );

  switch( networkMessage )
  {
    case NetMsgType::Snapshot:
    {
      ReadSnapshotBody( &reader, errors );
      TAC_HANDLE_ERROR( errors );
    } break;
      //case NetMsgType::Text: { ReadIncomingChatMessageBody( &reader, &mChat, errors ); } break;
  }
}


void ClientData::Update(
  float seconds,
  v2 inputDir,
  ClientSendNetworkMessageCallback sendNetworkMessageCallback,
  void* userData,
  Errors& errors )
{
  Vector< char > delayedMsg;
  while( mSavedNetworkMessages.TryPopSavedMessage( delayedMsg, mWorld->mElapsedSecs ) )
  {
    ExecuteNetMsg( delayedMsg.data(), ( int )delayedMsg.size(), errors );
    TAC_HANDLE_ERROR( errors );
  }

  Writer writer;
  writer.mFrom = GetEndianness();
  writer.mTo = GameEndianness;
  WriteNetMsgHeader( &writer, NetMsgType::Input );
  TAC_HANDLE_ERROR( errors );

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
  //  WriteOutgoingChatMessage( writer, &mChat, errors );
  //  TAC_HANDLE_ERROR( errors );
  //  sendNetworkMessageCallback(
  //    ( u8* )writer->mBuffer.mBytes,
  //    writer->mBuffer.mByteCountCur,
  //    userData );
  //}
}

void ClientData::ReceiveMessage(
  void* bytes,
  int byteCount,
  Errors& errors )
{
  if( mSavedNetworkMessages.mLagSimulationMS )
  {
    Vector< char > messageData( (char*)bytes, (char*)bytes + byteCount );
    mSavedNetworkMessages.SaveMessage( messageData, mWorld->mElapsedSecs );
    return;
  }
  ExecuteNetMsg( bytes, byteCount, errors );
}

}

