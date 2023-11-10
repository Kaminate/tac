#include "src/common/memory/tac_memory.h"
#include "src/common/core/tac_preprocessor.h"
#include "src/common/dataprocess/tac_settings.h"
#include "src/common/string/tac_string_util.h"
#include "src/space/tac_component.h"
#include "src/space/tac_entity.h"
#include "src/space/tac_player.h"
#include "src/space/tac_script.h"
#include "src/space/tac_script_game_client.h"
#include "src/space/tac_server.h"
#include "src/space/tac_space_net.h"
#include "src/space/tac_space_net.h"
#include "src/space/tac_world.h"
//#include <algorithm>
//#include <fstream>

namespace Tac
{
  // decreasing to 0 for easier debugging
  // ( consistant update between server/client )
  const float sSnapshotUntilNextSecondsMax = 0; //0.1f;

  typedef int ComponentRegistryEntryIndex;
  static char ComponentToBitField( ComponentRegistryEntryIndex componentType )
  {
    char result = 1 << ( char )componentType;
    return result;
  }

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
    auto player = SpawnPlayer();
    TAC_ASSERT( player );
    TAC_ASSERT( mOtherPlayers.size() < ServerData::sOtherPlayerCountMax );
    auto otherPlayer = new OtherPlayer();
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
    auto it = std::find_if( mOtherPlayers.begin(),
                            mOtherPlayers.end(),
                            [ & ]( OtherPlayer* otherPlayer )
                            { return otherPlayer->mConnectionUUID == connectionID; } );
    if( it == mOtherPlayers.end() )
      return;
    auto otherPlayer = *it;
    mWorld->KillPlayer( otherPlayer->mPlayerUUID );
    delete otherPlayer;
    mOtherPlayers.erase( it );
  }

  void ServerData::ReadInput( Reader* reader,
                              ConnectionUUID connectionID,
                              Errors& errors )
  {
    auto otherPlayer = FindOtherPlayer( connectionID );
    TAC_ASSERT( otherPlayer );

    auto player = mWorld->FindPlayer( otherPlayer->mPlayerUUID );
    if( !player )
      return;

    if( !reader->Read( &player->mInputDirection ) )
    {
      TAC_RAISE_ERROR( "failed to read player input direction", errors );
    }

    if( !reader->Read( &otherPlayer->mTimeStamp ) )
    {
      TAC_RAISE_ERROR( "failed to read player time stamp", errors );
    }
  }

  void ServerData::WriteSnapshotBody( OtherPlayer* otherPlayer, Writer* writer )
  {
    writer->Write( mWorld->mElapsedSecs );
    TAC_ASSERT( otherPlayer );
    auto oldWorld = mSnapshots.FindSnapshot( otherPlayer->mTimeStamp );
    if( !oldWorld )
      oldWorld = mEmptyWorld;

    writer->Write( otherPlayer->mTimeStamp );
    writer->Write( ( UUID )otherPlayer->mPlayerUUID );

    // Write deleted players
    {
      Vector< PlayerUUID > deletedPlayerUUIDs;
      for( Player* oldPlayer : oldWorld->mPlayers )
      {
        PlayerUUID playerUUID = oldPlayer->mPlayerUUID;
        Player* newPlayer = mWorld->FindPlayer( playerUUID );
        if( !newPlayer )
          deletedPlayerUUIDs.push_back( playerUUID );
      }

      writer->Write( ( PlayerCount )deletedPlayerUUIDs.size() );
      for( PlayerUUID playerUUID : deletedPlayerUUIDs )
      {
        writer->Write( playerUUID );
      }
    }

    // Write player differences
    {
      struct PlayerDifference
      {
        uint8_t    mBitfield;
        Player*    mNewPlayer;
        PlayerUUID playerUUID;
      };
      Vector< PlayerDifference > oldAndNewPlayers;

      for( Player* newPlayer : mWorld->mPlayers )
      {
        PlayerUUID playerUUID = newPlayer->mPlayerUUID;
        Player* oldPlayer = oldWorld->FindPlayer( playerUUID );
        uint8_t bitfield = GetNetworkBitfield( oldPlayer, newPlayer, PlayerNetworkBitsGet() );
        if( !bitfield )
          continue;

        const PlayerDifference diff
        {
          .mBitfield = bitfield,
          .mNewPlayer = newPlayer,
          .playerUUID = playerUUID,
        };
        oldAndNewPlayers.push_back( diff );
      }
      writer->Write( ( PlayerCount )oldAndNewPlayers.size() );

      for( PlayerDifference& diff : oldAndNewPlayers )
      {
        writer->Write( diff.playerUUID );
        writer->Write( diff.mNewPlayer, diff.mBitfield, PlayerNetworkBitsGet() );
      }
    }

    // Write deleted entites
    {
      Vector< EntityUUID > deletedEntityUUIDs;
      for( Entity* entity : oldWorld->mEntities )
      {
        EntityUUID entityUUID = entity->mEntityUUID;
        Entity* newEntity = mWorld->FindEntity( entityUUID );
        if( !newEntity )
          deletedEntityUUIDs.push_back( entityUUID );
      }

      writer->Write( ( EntityCount )deletedEntityUUIDs.size() );
      for( EntityUUID uuid : deletedEntityUUIDs )
        writer->Write( uuid );
    }

    // Write entity differenes
    {
      //const ComponentRegistry* componentRegistry = ComponentRegistry::Instance();
      const int registeredComponentCount = ComponentRegistry_GetComponentCount();


      struct EntityDifference
      {
        std::set< ComponentRegistryEntryIndex >       mDeletedComponents;
        std::map< ComponentRegistryEntryIndex, char > mChangedComponentBitfields;
        Entity*                                       mNewEntity = nullptr;
        EntityUUID                                    mEntityUUID = NullEntityUUID;
      };

      Vector< EntityDifference > entityDifferences;
      for( Entity* newEntity : mWorld->mEntities )
      {
        Entity* oldEntity = oldWorld->FindEntity( newEntity->mEntityUUID );

        std::set< ComponentRegistryEntryIndex >       deletedComponents;
        std::map< ComponentRegistryEntryIndex, char > changedComponentBitfields;

        for( int iComponentType = 0; iComponentType < registeredComponentCount; ++iComponentType )
        {
          auto componentType = ( ComponentRegistryEntryIndex )iComponentType;
          const ComponentRegistryEntry* componentData = ComponentRegistry_GetComponentAtIndex( iComponentType );
          Component* oldComponent = nullptr;
          if( oldEntity )
            oldComponent = oldEntity->GetComponent( componentData );

          Component* newComponent = newEntity->GetComponent( componentData );
          if( !oldComponent && !newComponent )
          {
            continue;
          }
          else if( oldComponent && !newComponent )
          {
            deletedComponents.insert( componentType );
          }
          else
          {
            uint8_t networkBitfield = GetNetworkBitfield( oldComponent,
                                                       newComponent,
                                                       componentData->mNetworkBits );
            if( networkBitfield )
              changedComponentBitfields[ componentType ] = networkBitfield;
          }
        }

        if( deletedComponents.empty() && changedComponentBitfields.empty() )
          continue;

        EntityDifference entityDifference;
        entityDifference.mDeletedComponents = deletedComponents;
        entityDifference.mChangedComponentBitfields = changedComponentBitfields;
        entityDifference.mNewEntity = newEntity;
        entityDifference.mEntityUUID = newEntity->mEntityUUID;
        entityDifferences.push_back( entityDifference );
      }

      writer->Write( ( EntityCount )entityDifferences.size() );

      for( const EntityDifference& entityDifference : entityDifferences )
      {
        writer->Write( entityDifference.mEntityUUID );

        char deletedComponentsBitfield = 0;
        for( ComponentRegistryEntryIndex componentType : entityDifference.mDeletedComponents )
          deletedComponentsBitfield |= ComponentToBitField( componentType );

        writer->Write( deletedComponentsBitfield );

        char changedComponentsBitfield = 0;
        for( auto pair : entityDifference.mChangedComponentBitfields )
          changedComponentsBitfield |= ComponentToBitField( pair.first );

        writer->Write( changedComponentsBitfield );

        for( int iComponentType = 0; iComponentType < registeredComponentCount; ++iComponentType )
        {
          if( !( changedComponentsBitfield & iComponentType ) )
            continue;

          auto componentType = ( ComponentRegistryEntryIndex )iComponentType;
          const ComponentRegistryEntry* componentRegistryEntry = ComponentRegistry_GetComponentAtIndex( iComponentType );
          Component* component = entityDifference.mNewEntity->GetComponent( componentRegistryEntry );
          char componentBitfield = entityDifference.mChangedComponentBitfields.at( componentType );
          writer->Write( component,
                         componentBitfield,
                         componentRegistryEntry->mNetworkBits );
        }
      }
    }
  }


  void ServerData::ExecuteNetMsg( const ConnectionUUID connectionID,
                                  const void* bytes,
                                  const int byteCount,
                                  Errors& errors )
  {
    Reader reader;
    reader.mBegin = bytes;
    reader.mEnd = ( char* )bytes + byteCount;
    reader.mFrom = GameEndianness;
    reader.mTo = GetEndianness();

    NetMsgType networkMessage = NetMsgType::Count;
    ReadNetMsgHeader( &reader, &networkMessage, errors );
    TAC_HANDLE_ERROR( errors );
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
    if( mWorld->mPlayers.size() >= sPlayerCountMax )
      return nullptr;
    return mWorld->SpawnPlayer( mPlayerUUIDCounter.AllocateNewUUID() );
  }


  void ServerData::ReceiveMessage( ConnectionUUID connectionID,
                                   void* bytes,
                                   int byteCount,
                                   Errors& errors )
  {
    auto otherPlayer = FindOtherPlayer( connectionID );
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
        ExecuteNetMsg( otherPlayer->mConnectionUUID, bytes, byteCount, errors );
        TAC_HANDLE_ERROR( errors );
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
    //  WriteOutgoingChatMessage( writer, &mChat, errors );
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


}

