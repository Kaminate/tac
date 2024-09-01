#pragma once

#include "tac-std-lib/containers/tac_optional.h"
#include "tac-std-lib/tac_ints.h"

#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/net/tac_space_net.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/world/tac_world.h"

namespace Tac
{
  struct ComponentRegistryEntry;

  // Represents the components owned by an entity
  struct ComponentRegistryBits
  {
    ComponentRegistryBits( Entity* = nullptr );
    void UnionWith( const ComponentRegistryEntry* );
    bool HasComponent( const ComponentRegistryEntry* ) const;
    u64  GetBitfield() const;
    bool operator == ( const ComponentRegistryBits& ) const = default;

  private:
    u64 Mask( const ComponentRegistryEntry* ) const;

    // Each bit of the bitfield represents an entry in the Component Registry
    u64 mBitfield {};
  };

  //bool operator == ( const ComponentRegistryBits&,  const ComponentRegistryBits&  );

  struct ChangedComponentBitfields
  {
    void       Set( const ComponentRegistryEntry*, NetVarDiff );
    NetVarDiff Get( const ComponentRegistryEntry* ) const;
    bool       IsDirty() const;

  private:
    // Each element in this array is a bitfield which represents dirty NetVars corresponding
    // to a single a component registry entry
    NetVarDiff mData[ 64 ]  {};
    bool       mDirty       {};
  };

  struct EntityMod
  {
    ChangedComponentBitfields mChangedComponentBitfields  {};
    ComponentRegistryBits     mComponents                 {};
    Entity*                   mEntity                     {};
  };

  struct EntityDiffs
  {
    static void Write( WorldsToDiff, WriteStream* writer );
    static void Read( World* , ReadStream* , Errors& );

  private:
    EntityDiffs( WorldsToDiff );
    void Write( WriteStream* );
    void WriteDeleted( WriteStream* );
    void WriteCreated( WriteStream* );
    void WriteModified( WriteStream* );
    void DiffEntities( EntitiesToDiff );
    static void ReadDeleted( World* , ReadStream* , Errors& );
    static void ReadCreated( World* , ReadStream* , Errors& );
    static void ReadModified( World* , ReadStream* , Errors& );

    Vector< Entity* >    mCreated;
    Vector< EntityUUID > mDestroyed;
    Vector< EntityMod >  mModified;
  };

} // namespace Tac

