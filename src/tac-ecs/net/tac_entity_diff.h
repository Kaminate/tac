#pragma once

#include "tac-std-lib/containers/tac_optional.h"
#include "tac-std-lib/tac_ints.h"

#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/net/tac_space_net.h"

namespace Tac
{
  struct ComponentRegistryEntry;
  struct Entity;

  // As a whole, this struct represents the components owned by an entity
  struct ComponentRegistryBits
  {
    ComponentRegistryBits(Entity* = nullptr);
    void UnionWith( const ComponentRegistryEntry* );
    bool HasComponent( const ComponentRegistryEntry* ) const;
    bool operator == ( const ComponentRegistryBits& ) const = default;
    u64 GetBitfield() const { return mBitfield; }

  private:
    u64 Mask( const ComponentRegistryEntry* ) const;

    // Each bit of the bitfield represents an entry in the Component Registry
    u64 mBitfield {};
  };

  //bool operator == ( const ComponentRegistryBits&,  const ComponentRegistryBits&  );

  struct ChangedComponentBitfields
  {
    void       Set( const ComponentRegistryEntry*, NetBitDiff );
    NetBitDiff Get( const ComponentRegistryEntry* ) const;
    bool       IsDirty() const;

  private:
    // Each element in this array is a bitfield which represents dirty NetVars corresponding
    // to a single a component registry entry
    NetBitDiff mData[ 64 ]  {};
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
    static void Write( World* oldWorld, World* newWorld, Writer* writer );
    static void Read( World* world, Reader* reader, Errors& errors );

  private:
    EntityDiffs( World* oldWorld, World* newWorld );
    void Write( Writer* );
    void WriteDeleted( Writer* );
    void WriteCreated( Writer* );
    void WriteModified( Writer* );
    void DiffEntities( Entity* oldEntity, Entity* newEntity );
    static void ReadDeleted( World* world, Reader* reader, Errors& errors );
    static void ReadCreated( World* world, Reader* reader, Errors& errors );
    static void ReadModified( World* world, Reader* reader, Errors& errors );

    Vector< Entity* >    mCreated;
    Vector< EntityUUID > mDestroyed;
    Vector< EntityMod >  mModified;
  };


  // Compares two entities, returns info about what has changed
  struct EntityDiff
  {
    // Needed for Created and Modified

    // Needed for Created, Modified, and Destroyed
    EntityUUID                mEntityUUID { NullEntityUUID };
  };

} // namespace Tac

