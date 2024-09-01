#pragma once

#include "tac-std-lib/tac_ints.h"
#include "tac-std-lib/meta/tac_meta_decl.h"

namespace Tac
{

  using UUID = u32;

  const UUID NullUUID {};

  enum class ConnectionUUID : UUID;
  const ConnectionUUID NullConnectionUUID { ( ConnectionUUID )NullUUID };

  enum class PlayerUUID : UUID;
  const PlayerUUID NullPlayerUUID { ( PlayerUUID )NullUUID };

  enum class EntityUUID : UUID;
  const EntityUUID NullEntityUUID { ( EntityUUID )NullUUID };

  using PlayerCount = u8;
  using EntityCount = u16;

  template < typename T >
  struct UUIDCounter
  {
    T AllocateNewUUID() { return ( T )( ++mUUIDCounter ); }
    UUID mUUIDCounter { NullUUID };
  };

  typedef UUIDCounter< EntityUUID > EntityUUIDCounter;
  typedef UUIDCounter< PlayerUUID > PlayerUUIDCounter;



  TAC_META_DECL( PlayerUUID );
  TAC_META_DECL( EntityUUID );



}

