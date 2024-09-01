#include "tac-ecs/net/tac_entity_diff.h" // self-inc

#include "tac-std-lib/containers/tac_optional.h"
#include "tac-std-lib/tac_ints.h"
#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/net/tac_space_net.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-std-lib/containers/tac_set.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-ecs/component/tac_component_registry.h" // GetIndex
#include "tac-ecs/entity/tac_entity.h" // GetComponent
#include "tac-ecs/component/tac_component.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-ecs/net/tac_space_net.h" // GetNetVarfield

namespace Tac
{

  struct ComponentInfo;

  // Represents the components owned by an entity
  struct ComponentRegistryBits
  {
    ComponentRegistryBits() = default;
    ComponentRegistryBits( Entity* );
    void UnionWith( const ComponentInfo* );
    bool HasComponent( const ComponentInfo* ) const;
    u64  GetBitfield() const;
    bool operator == ( const ComponentRegistryBits& ) const = default;

  private:
    u64 Mask( const ComponentInfo* ) const;

    // Each bit of the bitfield represents an entry in the Component Registry
    u64 mBitfield {};
  };

  //bool operator == ( const ComponentRegistryBits&,  const ComponentRegistryBits&  );

  struct ChangedComponentBitfields
  {
    void       Set( const ComponentInfo*, NetVarDiff );
    NetVarDiff Get( const ComponentInfo* ) const;
    bool       IsDirty() const;

  private:
    // Each element in this array is a bitfield which represents dirty NetVars corresponding
    // to a single a component registry entry
    //
    // Keyed by the component registration index, ie mData[ Model().GetEntry().GetIndex() ]
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


  // ----------------------------------------------------------------------------------------------

  u64 ComponentRegistryBits::Mask( const ComponentInfo* entry ) const
  {
    return ( u64 )1 << entry->GetIndex();
  }

  void ComponentRegistryBits::UnionWith( const ComponentInfo* entry )
  {
    mBitfield |= Mask( entry );
  }

  bool ComponentRegistryBits::HasComponent( const ComponentInfo* entry ) const
  {
    return mBitfield & Mask( entry );
  }

  ComponentRegistryBits::ComponentRegistryBits( Entity* entity )
  {
    for( const ComponentInfo& info : ComponentInfo::Iterate() )
      if( entity->HasComponent( &info ) )
        UnionWith( &info );
  }

  u64  ComponentRegistryBits::GetBitfield() const { return mBitfield; }

  // ----------------------------------------------------------------------------------------------

  void        ChangedComponentBitfields::Set( const ComponentInfo* entry,
                                              NetVarDiff data )
  {
    mData[ entry->GetIndex() ] = data;

    mDirty |= !data.Empty();
  }

  NetVarDiff  ChangedComponentBitfields::Get( const ComponentInfo* entry ) const
  {
    return mData[ entry->GetIndex() ];
  }

  bool        ChangedComponentBitfields::IsDirty() const
  {
    return mDirty;
  }

  // ----------------------------------------------------------------------------------------------

  EntityDiffs::EntityDiffs( WorldsToDiff worldDiff )
  {
    World* oldWorld{ worldDiff.mOldWorld };
    World* newWorld{ worldDiff.mNewWorld };

    Set< EntityUUID > entityUUIDs;

    for( Entity* entity : oldWorld->mEntities )
      entityUUIDs.insert( entity->mEntityUUID );

    for( Entity* entity : newWorld->mEntities )
      entityUUIDs.insert( entity->mEntityUUID );

    for( EntityUUID entityUUID : entityUUIDs )
    {
      Entity* oldEntity{ oldWorld->FindEntity( entityUUID ) };
      Entity* newEntity{ newWorld->FindEntity( entityUUID ) };
      const EntitiesToDiff entityDiff
      {
        .mOldEntity{oldEntity},
        .mNewEntity{newEntity},
      };
      DiffEntities( entityDiff );
    }
  }

  void EntityDiffs::DiffEntities(EntitiesToDiff entityDiff )
  {
    Entity* oldEntity{ entityDiff.mOldEntity };
    Entity* newEntity{ entityDiff.mNewEntity };

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

    for( const ComponentInfo& componentData : ComponentInfo::Iterate() )
    {
      if( !newComponents.HasComponent( &componentData ) )
        continue;

      const Component* oldComponent { oldEntity->GetComponent( &componentData ) };
      const Component* newComponent { newEntity->GetComponent( &componentData ) };
      const NetVarRegistration::DiffParams diffParams
      {
        .mOld{ oldComponent },
        .mNew{ newComponent },
      };
      const NetVarDiff componentDiff{ componentData.mNetVarRegistration.Diff( diffParams ) };

      changedComponentBitfields.Set( &componentData, componentDiff );
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


  void EntityDiffs::WriteDeleted( WriteStream* writer )
  {
    writer->Write( ( EntityCount )mDestroyed.size() );
    writer->WriteBytes( mDestroyed.data(), mDestroyed.size() * sizeof( EntityUUID ) );
  }

  void EntityDiffs::WriteCreated( WriteStream* writer )
  {
    NetVarDiff netVarDiff;
    netVarDiff.SetAll();

    writer->Write( ( EntityCount )mCreated.size() );
    for( Entity* entity : mCreated )
    {
      writer->Write( entity->mEntityUUID );
      writer->Write( ComponentRegistryBits( entity ) );
      for( const ComponentInfo& componentData : ComponentInfo::Iterate() )
        if( Component* component { entity->GetComponent( &componentData ) } )
          componentData.mNetVarRegistration.Write( writer, component, netVarDiff );
    }
  }

  void EntityDiffs::WriteModified( WriteStream* writer )
  {
    writer->Write( ( EntityCount )mModified.size() );
    for( const EntityMod& mod : mModified )
    {
      Entity* entity{ mod.mEntity };
      writer->Write( entity->mEntityUUID );
      writer->Write( ComponentRegistryBits( entity ) );
      for( const ComponentInfo& componentData : ComponentInfo::Iterate() )
      {
        Component* component{ entity->GetComponent( &componentData ) };
        if( !component )
          continue;

        const NetVarDiff netVarDiff{ mod.mChangedComponentBitfields.Get( &componentData ) };
        componentData.mNetVarRegistration.Write( writer, component, netVarDiff );
      }
    }
  }

  void EntityDiffs::Write( WriteStream* writer )
  {
    WriteDeleted(writer);
    WriteCreated(writer);
    WriteModified(writer);
  }


  void EntityDiffs::ReadDeleted( World* world, ReadStream* reader, Errors& errors )
  {
    const auto numDeletedEntities = TAC_CALL( reader->Read<EntityCount>( errors ) );
    for( EntityCount i {}; i < numDeletedEntities; ++i )
    {
      TAC_CALL( const auto entityUUID { reader->Read<EntityUUID >( errors )  });
      world->KillEntity( entityUUID );
    }
  }

  void EntityDiffs::ReadCreated( World* world, ReadStream* reader, Errors& errors )
  {
    TAC_CALL( const auto n { reader->Read<EntityCount>( errors )  });
    for( EntityCount i {}; i < n; ++i )
    {
      TAC_CALL( const EntityUUID entityUUID{ reader->Read< EntityUUID >( errors ) } );
      TAC_CALL( const ComponentRegistryBits componentRegistryBits{
        reader->Read< ComponentRegistryBits >( errors ) } );

      Entity* entity { world->SpawnEntity( entityUUID ) };
      for( const ComponentInfo& componentInfo : ComponentInfo::Iterate() )
      {
        if( componentRegistryBits.HasComponent( &componentInfo ) )
        {
          Component* component { entity->AddNewComponent( &componentInfo ) };
          component->PreReadDifferences();
          TAC_CALL( componentInfo.mNetVarRegistration.Read( reader, component, errors ) );
          component->PostReadDifferences();
        }
      }
    }
  }

  void EntityDiffs::ReadModified( World* world, ReadStream* reader, Errors& errors )
  {
    TAC_CALL( const auto n{ reader->Read<EntityCount >( errors ) } );
    for( EntityCount i {}; i < n; ++i )
    {
      TAC_CALL( const auto entityUUID{ reader->Read<EntityUUID >( errors ) } );

      Entity* entity { world->FindEntity( entityUUID ) };

      const ComponentRegistryBits oldComponents( entity );
      TAC_CALL( const ComponentRegistryBits newComponents{
        reader->Read< ComponentRegistryBits >( errors ) } );

      for( const ComponentInfo& componentInfo : ComponentInfo::Iterate() )
      {
        const bool had { oldComponents.HasComponent( &componentInfo ) };
        const bool has { newComponents.HasComponent( &componentInfo ) };
        if( !had && !has )
          continue;

        if( had && !has )
        {
          entity->RemoveComponent( &componentInfo );
          continue;
        }

        Component* component{ has
          ? entity->GetComponent( &componentInfo )
        : entity->AddNewComponent( &componentInfo ) };

        component->PreReadDifferences();
        TAC_CALL( componentInfo.mNetVarRegistration.Read( reader, component, errors ) );
        component->PostReadDifferences();
      }
    }
  }

  // ----------------------------------------------------------------------------------------------


} // namespace Tac

void Tac::EntityDiffAPI::Read( World* world, ReadStream* reader, Errors& errors )
{
  TAC_CALL( EntityDiffs::ReadDeleted( world, reader, errors ) );
  TAC_CALL( EntityDiffs::ReadCreated( world, reader, errors ) );
  TAC_CALL( EntityDiffs::ReadModified( world, reader, errors ) );
}

void Tac::EntityDiffAPI::Write( WorldsToDiff worldDiff, WriteStream* writer )
{
  EntityDiffs diffs( worldDiff );
  diffs.Write( writer );
}




