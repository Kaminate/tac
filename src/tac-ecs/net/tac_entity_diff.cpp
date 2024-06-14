#include "tac-ecs/net/tac_entity_diff.h" // self-inc

#include "tac-std-lib/containers/tac_set.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-ecs/component/tac_component_registry.h" // GetIndex
#include "tac-ecs/entity/tac_entity.h" // GetComponent
#include "tac-ecs/component/tac_component.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-ecs/net/tac_space_net.h" // GetNetVarfield

namespace Tac
{
  // ----------------------------------------------------------------------------------------------

  u64 ComponentRegistryBits::Mask( const ComponentRegistryEntry* entry ) const
  {
    return ( u64 )1 << entry->GetIndex();
  }

  void ComponentRegistryBits::UnionWith( const ComponentRegistryEntry* entry )
  {
    mBitfield |= Mask( entry );
  }

  bool ComponentRegistryBits::HasComponent( const ComponentRegistryEntry* entry ) const
  {
    return mBitfield & Mask( entry );
  }

  ComponentRegistryBits::ComponentRegistryBits( Entity* entity )
  {
    if( entity )
      for( const ComponentRegistryEntry& entry : ComponentRegistryIterator() )
        if( entity->HasComponent( &entry ) )
          UnionWith( &entry );
  }

  // ----------------------------------------------------------------------------------------------

  void        ChangedComponentBitfields::Set( const ComponentRegistryEntry* entry,
                                              NetBitDiff data )
  {
    mData[ entry->GetIndex() ] = data;

    mDirty |= !data.Empty();
  }

  NetBitDiff  ChangedComponentBitfields::Get( const ComponentRegistryEntry* entry ) const
  {
    return mData[ entry->GetIndex() ];
  }

  bool        ChangedComponentBitfields::IsDirty() const
  {
    return mDirty;
  }

  // ----------------------------------------------------------------------------------------------

  EntityDiffs::EntityDiffs( World* oldWorld, World* newWorld )
  {
    Set< EntityUUID > entityUUIDs;

    for( Entity* entity : oldWorld->mEntities )
      entityUUIDs.insert( entity->mEntityUUID );

    for( Entity* entity : newWorld->mEntities )
      entityUUIDs.insert( entity->mEntityUUID );

    for( EntityUUID entityUUID : entityUUIDs )
    {
      Entity* oldEntity = oldWorld->FindEntity( entityUUID );
      Entity* newEntity = newWorld->FindEntity( entityUUID );
      DiffEntities( oldEntity, newEntity );
    }
  }

  void EntityDiffs::DiffEntities( Entity* oldEntity, Entity* newEntity )
  {
    if( oldEntity && !newEntity )
    {
      mDestroyed.push_back(oldEntity->mEntityUUID );
      return;
    }

    if( !oldEntity && newEntity )
    {
      mCreated.push_back( newEntity );
      return;
    }

    const ComponentRegistryBits oldComponents( oldEntity );
    const ComponentRegistryBits newComponents( newEntity );

    ChangedComponentBitfields changedComponentBitfields;

    for( const ComponentRegistryEntry& componentData : ComponentRegistryIterator() )
    {
      if( !newComponents.HasComponent( &componentData ) )
        continue;

      const Component* oldComponent { oldEntity->GetComponent( &componentData ) };
      const Component* newComponent { newEntity->GetComponent( &componentData ) };
      const NetBitDiff netBit{ GetNetVarfield( oldComponent,
                                                    newComponent,
                                                    componentData.mNetVars ) };

      changedComponentBitfields.Set( &componentData, netBit );
    }

    if( oldComponents == newComponents && !changedComponentBitfields.IsDirty() )
      return;

    const EntityMod mod
    {
      .mChangedComponentBitfields { changedComponentBitfields },
      .mComponents                { newComponents },
      .mEntity                    { newEntity },
    };

    mModified.push_back( mod );
  }

  void EntityDiffs::Write( World* oldWorld, World* newWorld, Writer* writer )
  {
    EntityDiffs diffs( oldWorld, newWorld );
    diffs.Write( writer );
  }

  void EntityDiffs::WriteDeleted( Writer* writer )
  {
    writer->Write( ( EntityCount )mDestroyed.size() );
    for( EntityUUID entityUUID : mDestroyed )
        writer->Write( entityUUID );
  }

  void EntityDiffs::WriteCreated( Writer* writer )
  {
    writer->Write( ( EntityCount )mCreated.size() );
    for( Entity* entity : mCreated )
    {
      writer->Write( entity->mEntityUUID );
      writer->Write( ComponentRegistryBits(entity) );
      for( const ComponentRegistryEntry& componentData : ComponentRegistryIterator() )
        if( Component* component = entity->GetComponent( &componentData ) )
          writer->Write( component,
                         NetBitDiff{ 0xff },
                         componentData.mNetVars );
    }
  }

  void EntityDiffs::WriteModified( Writer* writer )
  {
    writer->Write( ( EntityCount )mModified.size() );
    for( const EntityMod& mod : mModified )
    {
      writer->Write( mod.mEntity->mEntityUUID );
      writer->Write( ComponentRegistryBits( mod.mEntity ) );
      for( const ComponentRegistryEntry& componentData : ComponentRegistryIterator() )
        if( Component* component = mod.mEntity->GetComponent( &componentData ) )
          writer->Write( component,
                         mod.mChangedComponentBitfields.Get( &componentData ),
                         componentData.mNetVars );
    }
  }

  void EntityDiffs::Write( Writer* writer )
  {
    WriteDeleted(writer);
    WriteCreated(writer);
    WriteModified(writer);
  }

  void EntityDiffs::Read( World* world, Reader* reader, Errors& errors )
  {
    TAC_CALL( ReadDeleted( world, reader, errors ) );
    TAC_CALL( ReadCreated( world, reader, errors ) );
    TAC_CALL( ReadModified( world, reader, errors ) );

  }

  void EntityDiffs::ReadDeleted( World* world, Reader* reader, Errors& errors )
  {
    const auto numDeletedEntities = TAC_CALL( reader->Read<EntityCount>( errors ) );
    for( EntityCount i { 0 }; i < numDeletedEntities; ++i )
    {
      const auto entityUUID = TAC_CALL( reader->Read<EntityUUID >( errors ) );
      world->KillEntity( entityUUID );
    }
  }

  void EntityDiffs::ReadCreated( World* world, Reader* reader, Errors& errors )
  {
    TAC_CALL( const auto n { reader->Read<EntityCount>( errors )  });
    for( EntityCount i { 0 }; i < n; ++i )
    {
      TAC_CALL( const auto entityUUID { reader->Read<EntityUUID>( errors ) });
      TAC_CALL( const auto componentRegistryBits { reader->Read<ComponentRegistryBits>( errors ) });

      Entity* entity { world->SpawnEntity( entityUUID ) };
      for( const ComponentRegistryEntry& componentRegistryEntry : ComponentRegistryIterator() )
      {
        if( componentRegistryBits.HasComponent( &componentRegistryEntry ) )
        {
          Component* component { entity->AddNewComponent( &componentRegistryEntry ) };
          component->PreReadDifferences();
          TAC_CALL( reader->Read( component, componentRegistryEntry.mNetVars, errors ) );
          component->PostReadDifferences();
        }
      }
    }
  }

  void EntityDiffs::ReadModified( World* world, Reader* reader, Errors& errors )
  {
    TAC_CALL( const auto n{ reader->Read<EntityCount >( errors ) } );
    for( EntityCount i = 0; i < n; ++i )
    {
      TAC_CALL( const auto entityUUID{ reader->Read<EntityUUID >( errors ) } );

      Entity* entity { world->FindEntity( entityUUID ) };

      const auto oldComponents { ComponentRegistryBits( entity ) };
      TAC_CALL( const auto newComponents{ reader->Read<ComponentRegistryBits>( errors ) } );

      for( const ComponentRegistryEntry& componentRegistryEntry : ComponentRegistryIterator() )
      {
        const bool had { oldComponents.HasComponent( &componentRegistryEntry ) };
        const bool has { newComponents.HasComponent( &componentRegistryEntry ) };
        if( !had && !has )
          continue;

        if( had && !has )
        {
          entity->RemoveComponent( &componentRegistryEntry );
          continue;
        }

        Component* component{ has
          ? entity->GetComponent( &componentRegistryEntry )
        : entity->AddNewComponent( &componentRegistryEntry ) };

        component->PreReadDifferences();
        TAC_CALL( reader->Read( component, componentRegistryEntry.mNetVars, errors ) );
        component->PostReadDifferences();
      }
    }
  }

  // ----------------------------------------------------------------------------------------------


} // namespace Tac



