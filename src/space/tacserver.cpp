#include "tacserver.h"
#include "common/tacPreprocessor.h"
#include "tacplayer.h"
#include "tacspacenet.h"
#include "tacworld.h"
#include "tacentity.h"
#include "taccomponent.h"
#include "tacspacenet.h"
#include "common/tacUtility.h"
#include "tacscript.h"
#include "tacscriptgameclient.h"
#include "common/tacSettings.h"
#include "common/tacMemory.h"

#include <algorithm>
#include <fstream>

// decreasing to 0 for easier debugging
// ( consistant update between server/client )
const float sSnapshotUntilNextSecondsMax = 0; //0.1f;

typedef int TacComponentRegistryEntryIndex;
static char TacComponentToBitField( TacComponentRegistryEntryIndex componentType )
{
  char result = 1 << ( char )componentType;
  return result;
}

TacServerData::TacServerData()
{
  mWorld = new TacWorld();
  mEmptyWorld = new TacWorld();
}

TacServerData::~TacServerData()
{
  for( auto otherPlayer : mOtherPlayers )
    delete otherPlayer;
  delete mWorld;
  delete mEmptyWorld;
}

void TacServerData::DebugImgui()
{
  if( mWorld )
  {
    mWorld->DebugImgui();
  }
}
void TacServerData::OnClientJoin( TacConnectionUUID connectionID )
{
  auto player = SpawnPlayer();
  TacAssert( player );
  TacAssert( mOtherPlayers.size() < TacServerData::sOtherPlayerCountMax );
  auto otherPlayer = new TacOtherPlayer();
  otherPlayer->mPlayerUUID = player->mPlayerUUID;
  otherPlayer->mConnectionUUID = connectionID;
  mOtherPlayers.push_back( otherPlayer );
}


TacOtherPlayer* TacServerData::FindOtherPlayer( TacConnectionUUID connectionID )
{
  for( auto otherPlayer : mOtherPlayers )
    if( otherPlayer->mConnectionUUID == connectionID )
      return otherPlayer;
  return nullptr;
}

void TacServerData::OnLoseClient( TacConnectionUUID connectionID )
{
  auto it = std::find_if(
    mOtherPlayers.begin(),
    mOtherPlayers.end(),
    [ & ]( TacOtherPlayer* otherPlayer ) { return otherPlayer->mConnectionUUID == connectionID; } );
  if( it == mOtherPlayers.end() )
    return;
  auto otherPlayer = *it;
  mWorld->KillPlayer( otherPlayer->mPlayerUUID );
  delete otherPlayer;
  mOtherPlayers.erase( it );
}

void TacServerData::ReadInput(
  TacReader* reader,
  TacConnectionUUID connectionID,
  TacErrors& errors )
{
  auto otherPlayer = FindOtherPlayer( connectionID );
  TacAssert( otherPlayer );

  auto player = mWorld->FindPlayer( otherPlayer->mPlayerUUID );
  if( !player )
    return;

  if( !reader->Read( &player->mInputDirection ) )
  {
    errors += "fuck";
    return;
  }

  if( !reader->Read( &otherPlayer->mTimeStamp ) )
  {
    errors += "fuck";
    return;
  }
}

void TacServerData::WriteSnapshotBody( TacOtherPlayer* otherPlayer, TacWriter* writer )
{
  writer->Write( mWorld->mElapsedSecs );
  TacAssert( otherPlayer );
  auto oldWorld = mSnapshots.FindSnapshot( otherPlayer->mTimeStamp );
  if( !oldWorld )
    oldWorld = mEmptyWorld;

  writer->Write( otherPlayer->mTimeStamp );
  writer->Write( ( TacUUID )otherPlayer->mPlayerUUID );

  // Write deleted players
  {
    TacVector< TacPlayerUUID > deletedPlayerUUIDs;
    for( auto oldPlayer : oldWorld->mPlayers )
    {
      auto playerUUID = oldPlayer->mPlayerUUID;
      auto newPlayer = mWorld->FindPlayer( playerUUID );
      if( !newPlayer )
        deletedPlayerUUIDs.push_back( playerUUID );
    }
    writer->Write( ( TacPlayerCount )deletedPlayerUUIDs.size() );
    for( TacPlayerUUID playerUUID:deletedPlayerUUIDs )
    {
      writer->Write( playerUUID );
    }
  }

  // Write player differences
  {
    struct TacPlayerDifference
    {
      uint8_t mBitfield;
      TacPlayer* mNewPlayer;
      TacPlayerUUID playerUUID;
    };
    TacVector< TacPlayerDifference > oldAndNewPlayers;

    for( auto newPlayer : mWorld->mPlayers )
    {
      auto playerUUID = newPlayer->mPlayerUUID;
      auto oldPlayer = oldWorld->FindPlayer( playerUUID );
      auto bitfield = GetNetworkBitfield( oldPlayer, newPlayer, TacPlayerBits );
      if( !bitfield )
        continue;

      TacPlayerDifference diff;
      diff.mBitfield = bitfield;
      diff.mNewPlayer = newPlayer;
      diff.playerUUID = playerUUID;
      oldAndNewPlayers.push_back( diff );
    }
    writer->Write( ( TacPlayerCount )oldAndNewPlayers.size() );

    for( TacPlayerDifference& diff : oldAndNewPlayers )
    {
      writer->Write( diff.playerUUID );
      writer->Write( diff.mNewPlayer, diff.mBitfield, TacPlayerBits );
    }
  }

  // Write deleted entites
  {
    TacVector< TacEntityUUID > deletedEntityUUIDs;
    for( auto entity : oldWorld->mEntities )
    {
      auto entityUUID = entity->mEntityUUID;
      auto newEntity = mWorld->FindEntity( entityUUID );
      if( !newEntity )
        deletedEntityUUIDs.push_back( entityUUID );
    }
    writer->Write( ( TacEntityCount )deletedEntityUUIDs.size() );
    for( TacEntityUUID uuid:deletedEntityUUIDs )
      writer->Write( uuid );
  }

  // Write entity differenes
  {
    TacComponentRegistry* componentRegistry = TacComponentRegistry::Instance();
    int registeredComponentCount = componentRegistry->mEntries.size();


    struct TacEntityDifference
    {
      std::set< TacComponentRegistryEntryIndex > mDeletedComponents;
      std::map< TacComponentRegistryEntryIndex, char > mChangedComponentBitfields;
      TacEntity* mNewEntity = nullptr;
      TacEntityUUID mEntityUUID = TacNullEntityUUID;
    };

    TacVector< TacEntityDifference > entityDifferences;
    for( auto newEntity : mWorld->mEntities )
    {
      auto oldEntity = oldWorld->FindEntity( newEntity->mEntityUUID );

      std::set< TacComponentRegistryEntryIndex > deletedComponents;
      std::map< TacComponentRegistryEntryIndex, char > changedComponentBitfields;

      for( int iComponentType = 0; iComponentType < registeredComponentCount; ++iComponentType )
      {
        auto componentType = ( TacComponentRegistryEntryIndex )iComponentType;
        TacComponentRegistryEntry* componentData = componentRegistry->mEntries[ iComponentType ];
        TacComponent* oldComponent = nullptr;
        if( oldEntity )
          oldComponent = oldEntity->GetComponent( componentData );
        auto newComponent = newEntity->GetComponent( componentData );
        if( !oldComponent && !newComponent )
          continue;
        else if( oldComponent && !newComponent )
          deletedComponents.insert( componentType );
        else
        {
          auto networkBitfield = GetNetworkBitfield(
            oldComponent,
            newComponent,
            componentData->mNetworkBits );
          if( networkBitfield )
            changedComponentBitfields[ componentType ] = networkBitfield;
        }
      }

      if( deletedComponents.empty() && changedComponentBitfields.empty() )
        continue;

      TacEntityDifference entityDifference;
      entityDifference.mDeletedComponents = deletedComponents;
      entityDifference.mChangedComponentBitfields = changedComponentBitfields;
      entityDifference.mNewEntity = newEntity;
      entityDifference.mEntityUUID = newEntity->mEntityUUID;
      entityDifferences.push_back( entityDifference );
    }

    writer->Write( ( TacEntityCount )entityDifferences.size() );

    for( const TacEntityDifference& entityDifference : entityDifferences )
    {
      writer->Write( entityDifference.mEntityUUID );

      char deletedComponentsBitfield = 0;
      for( auto componentType : entityDifference.mDeletedComponents )
        deletedComponentsBitfield |= TacComponentToBitField( componentType );
      writer->Write( deletedComponentsBitfield );

      char changedComponentsBitfield = 0;
      for( auto pair : entityDifference.mChangedComponentBitfields )
        changedComponentsBitfield |= TacComponentToBitField( pair.first );
      writer->Write( changedComponentsBitfield );

      for( int iComponentType = 0; iComponentType < registeredComponentCount; ++iComponentType )
      {
        if( !( changedComponentsBitfield & iComponentType ) )
          continue;
        auto componentType = ( TacComponentRegistryEntryIndex )iComponentType;
        TacComponentRegistryEntry* componentRegistryEntry = componentRegistry->mEntries[ iComponentType ];
        TacComponent* component = entityDifference.mNewEntity->GetComponent( componentRegistryEntry );
        auto componentBitfield = entityDifference.mChangedComponentBitfields.at( componentType );
        writer->Write(
          component,
          componentBitfield,
          componentRegistryEntry->mNetworkBits );
      }
    }
  }
}


void TacServerData::ExecuteNetMsg(
  TacConnectionUUID connectionID,
  void* bytes,
  int byteCount,
  TacErrors& errors )
{
  TacReader reader;
  reader.mBegin = bytes;
  reader.mEnd = ( char* )bytes + byteCount;
  reader.mFrom = TacGameEndianness;
  reader.mTo = TacGetEndianness();

  auto networkMessage = TacReadNetMsgHeader( &reader, errors );
    TAC_HANDLE_ERROR( errors );
  switch( networkMessage )
  {
    case TacNetMsgType::Input:
      ReadInput( &reader, connectionID, errors );
      break;
      //case TacNetMsgType::Text:
      //{
      //  TacReadIncomingChatMessageBody(
      //    &reader,
      //    &mChat,
      //    errors );
      //} break;
  }
}

TacEntity* TacServerData::SpawnEntity()
{
  mEntityUUIDCounter = ( TacEntityUUID )( ( TacUUID )mEntityUUIDCounter + 1 );
  auto entity = mWorld->SpawnEntity( mEntityUUIDCounter );
  return entity;
}

TacPlayer* TacServerData::SpawnPlayer()
{
  if( mWorld->mPlayers.size() >= sPlayerCountMax )
    return nullptr;
  mPlayerUUIDCounter = ( TacPlayerUUID )( ( TacUUID )mPlayerUUIDCounter + 1 );
  return mWorld->SpawnPlayer( mPlayerUUIDCounter );
}


void TacServerData::ReceiveMessage(
  TacConnectionUUID connectionID,
  void* bytes,
  int byteCount,
  TacErrors& errors )
{
  auto otherPlayer = FindOtherPlayer( connectionID );
  if( !otherPlayer )
    return;

  if( otherPlayer->delayedNetMsg.mLagSimulationMS )
  {
    otherPlayer->delayedNetMsg.SaveMessage( TacTemporaryMemory( bytes, byteCount ), mWorld->mElapsedSecs );
    return;
  }

  ExecuteNetMsg(
    connectionID,
    bytes,
    byteCount,
    errors );
}

void TacServerData::Update(
  float seconds,
  ServerSendNetworkMessageCallback sendNetworkMessageCallback,
  void* userData,
  TacErrors& errors )
{
  for( auto otherPlayer : mOtherPlayers )
  {
    TacVector< char > savedNetMsg;
    while( otherPlayer->delayedNetMsg.TryPopSavedMessage( savedNetMsg, mWorld->mElapsedSecs ) )
    {
      ExecuteNetMsg(
        otherPlayer->mConnectionUUID,
        savedNetMsg.data(),
        ( int )savedNetMsg.size(),
        errors );
      TAC_HANDLE_ERROR( errors );
    }
  }


  mWorld->Step( seconds );

  mSnapshotUntilNextSecondsCur -= seconds;
  if( mSnapshotUntilNextSecondsCur > 0 )
    return;
  mSnapshotUntilNextSecondsCur =
    sSnapshotUntilNextSecondsMax;

  mSnapshots.AddSnapshot( mWorld );

  for( auto otherPlayer : mOtherPlayers )
  {
    TacWriter writer;
    writer.mFrom = TacGetEndianness();
    writer.mTo = TacGameEndianness;
    TacWriteNetMsgHeader( &writer, TacNetMsgType::Snapshot );
    WriteSnapshotBody( otherPlayer, &writer );
    if( sendNetworkMessageCallback )
    {
      sendNetworkMessageCallback(
        otherPlayer->mConnectionUUID,
        writer.mBytes.data(),
        ( int )writer.mBytes.size(),
        userData );
    }
  }

  //if( mChat.mIsReadyToSend )
  //{
  //  // todo: keep sending until the client has acknowled our message
  //  mChat.mIsReadyToSend = false;
  //  writer->mBuffer = buffer;
  //  TacWriteOutgoingChatMessage( writer, &mChat, errors );
  //  TAC_HANDLE_ERROR( errors );
  //  for( auto otherPlayer : mOtherPlayers )
  //  {
  //    sendNetworkMessageCallback(
  //      otherPlayer->mConnectionUUID,
  //      ( u8* )writer->mBuffer.mBytes,
  //      writer->mBuffer.mByteCountCur,
  //      userData );
  //  }
  //}
}

