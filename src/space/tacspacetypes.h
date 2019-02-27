#pragma once
#include <cstdint>

enum class TacComponentType
{
  Say,
  Model,
  Collider,
  Terrain,

  Count
};

inline const char* TacToString( TacComponentType componentType )
{
  switch( componentType )
  {
  case TacComponentType::Say: return "say";
  case TacComponentType::Model: return "model";
  case TacComponentType::Collider: return "collider";
  case TacComponentType::Terrain: return "terrain";
  }
  return nullptr;
}

enum class TacSystemType
{
  Graphics,
  Physics,

  Count
};

typedef uint32_t TacUUID;
const TacUUID TacNullUUID = 0;

enum class TacConnectionUUID : TacUUID;
const TacConnectionUUID TacNullConnectionUUID = ( TacConnectionUUID )TacNullUUID;

enum class TacPlayerUUID : TacUUID;
const TacPlayerUUID TacNullPlayerUUID = ( TacPlayerUUID )TacNullUUID;

enum class TacEntityUUID : TacUUID;
const TacEntityUUID TacNullEntityUUID = ( TacEntityUUID )TacNullUUID;

enum class TacAssetUUID : TacUUID;
const TacAssetUUID TacNullAssetUUID = ( TacAssetUUID )TacNullUUID;

typedef uint8_t TacPlayerCount;
typedef uint16_t TacEntityCount;

