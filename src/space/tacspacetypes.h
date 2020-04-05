
#pragma once
#include <cstdint>
namespace Tac
{


typedef uint32_t UUID;
const UUID NullUUID = 0;

enum class ConnectionUUID : UUID;
const ConnectionUUID NullConnectionUUID = ( ConnectionUUID )NullUUID;

enum class PlayerUUID : UUID;
const PlayerUUID NullPlayerUUID = ( PlayerUUID )NullUUID;

enum class EntityUUID : UUID;
const EntityUUID NullEntityUUID = ( EntityUUID )NullUUID;

enum class AssetUUID : UUID;
const AssetUUID NullAssetUUID = ( AssetUUID )NullUUID;

typedef uint8_t PlayerCount;
typedef uint16_t EntityCount;


}

