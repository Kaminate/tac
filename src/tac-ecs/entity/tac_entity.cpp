#include "tac_entity.h" // self-inc

#include "tac-ecs/component/tac_component.h"
#include "tac-ecs/component/tac_component_registry.h"
#include "tac-ecs/system/tac_system.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/meta/tac_meta_composite.h"
#include "tac-std-lib/meta/tac_meta.h"
#include "tac-std-lib/error/tac_assert.h"

namespace Tac
{
  static v3 Vector3FromJson( Json& json )
  {
    v3 v 
    {
      ( float )json[ "x" ].mNumber,
      ( float )json[ "y" ].mNumber,
      ( float )json[ "z" ].mNumber,
    };
    return v;
  }

  static Json* Vector3ToJson( v3 v )
  {
    static Json json;
    json.GetChild( "x" ).SetNumber( v.x );
    json.GetChild( "y" ).SetNumber( v.y );
    json.GetChild( "z" ).SetNumber( v.z );
    return &json;
  }
}

namespace Tac
{
  // -----------------------------------------------------------------------------------------------

  Entity::~Entity()
  {
    RemoveAllComponents();
    // Assert that the world no longer contains this entity?
  }

  void Entity::RemoveAllComponents()
  {
    for( Component* component : mComponents )
    {
      const ComponentInfo* entry { component->GetEntry() };
      entry->mDestroyFn( mWorld, component );
    }
    mComponents.Clear();
  }

  auto Entity::AddNewComponent( const ComponentInfo* entry ) -> Component*
  {
    TAC_ASSERT( entry );
    TAC_ASSERT( !HasComponent( entry ) );
    TAC_ASSERT( entry->mCreateFn );
    Component* component { entry->mCreateFn( mWorld ) };
    TAC_ASSERT( component );
    mComponents.Add( component );
    component->mEntity = this;
    return component;
  }

  auto Entity::GetComponent( const ComponentInfo* entry ) dynmc -> dynmc Component*
  {
    for( Component* component : mComponents )
      if( component->GetEntry() == entry )
        return component;
    return nullptr;
  }

  auto Entity::GetComponent( const ComponentInfo* entry ) const -> const Component*
  {
    for( const Component* component : mComponents )
      if( component->GetEntry() == entry )
        return component;
    return nullptr;
  }

  bool Entity::HasComponent( const ComponentInfo* entry ) const
  {
    return GetComponent( entry ) != nullptr;
  }

  void Entity::RemoveComponent( const ComponentInfo* entry )
  {
    Component* component { mComponents.Remove( entry ) };
    entry->mDestroyFn( mWorld, component );
  }

  void Entity::DeepCopy( const Entity& entity )
  {
    TAC_ASSERT( mWorld );
    TAC_ASSERT( entity.mWorld );
    TAC_ASSERT( mWorld != entity.mWorld );

    mEntityUUID = entity.mEntityUUID;
    mName = entity.mName;
    mInheritParentScale = entity.mInheritParentScale;
    mRelativeSpace = entity.mRelativeSpace;
    mWorldPosition = entity.mWorldPosition;
    mWorldTransform = entity.mWorldTransform;
    mInheritParentScale = entity.mInheritParentScale;
    mActive = entity.mActive;
    mStatic = entity.mStatic;

    RemoveAllComponents();

    for( Entity* child : mChildren )
      mWorld->KillEntity( child );

    mChildren.clear();

    for( const Component* oldComponent : entity.mComponents )
    {
      const ComponentInfo* entry{ oldComponent->GetEntry() };
      Component* newComponent { AddNewComponent( entry ) };
      newComponent->CopyFrom( oldComponent );
    }

    // shouldn't this fn be recursive?
  }

  void Entity::AddChild( Entity* child )
  {
    TAC_ASSERT( !Contains( mChildren, child ) );
    TAC_ASSERT( child->mParent != this );
    child->Unparent();
    child->mParent = this;
    mChildren.push_back( child );
  }

  void Entity::DebugImgui()
  {
    // This has been moved to tac_level_editor_property_window.cpp EntityImGui() (?)
#if 0
    ImGui::PushID( this );
    OnDestruct( ImGui::PopID() );

    const auto headerName = ShortFixedString::Concant( "Entity id ", ToString( mEntityUUID );
    if( !ImGui::CollapsingHeader( headerName, ImGuiTreeNodeFlags_DefaultOpen ) )
      return;

    ImGui::Indent();
    OnDestruct( ImGui::Unindent() );
    ImGui::DragFloat3( "Position", mPosition.data(), 0.1f );

    Vector< ComponentInfoIndex > couldBeAdded;
    for( int i{}; i < ( int )ComponentInfoIndex::Count; i++ )
    {
      auto componentType = ( ComponentInfoIndex )i;
      if( HasComponent( componentType ) )
        continue;

      auto componentData = GetComponentData( componentType );
      if( !componentData )
        continue;
      couldBeAdded.push_back( componentType );
    }

    if( !couldBeAdded.empty() && ImGui::BeginMenu( "Add Component" ) )
    {
      for( ComponentInfoIndex componentType : couldBeAdded )
      {
        auto componentData = GetComponentData( componentType );
        if( !ImGui::MenuItem( componentData->mName ) )
          continue;

        AddNewComponent( componentType );
        break;
      }

      ImGui::EndMenu();
    }

    if( !mComponents.empty() && ImGui::BeginMenu( "Remove Component" ) )
    {
      for( auto component : mComponents )
      {
        auto componentType = component->GetComponentType();
        auto componentData = GetComponentData( componentType );
        if( !ImGui::MenuItem( componentData->mName ) )
          continue;
        RemoveComponent( componentType );
        break;
      }
      ImGui::EndMenu();
    }
    for( auto component : mComponents )
      component->DebugImgui();
#endif
  }

  void Entity::Unparent()
  {
    if( !mParent )
      return;

    for( int iChild {}; iChild < mParent->mChildren.size(); ++iChild )
    {
      if( mParent->mChildren[ iChild ] == this )
      {
        mParent->mChildren[ iChild ] = mParent->mChildren[ mParent->mChildren.size() - 1 ];
        mParent->mChildren.pop_back();
        break;
      }
    }

    mParent = nullptr;
    // todo: relative positioning
  }

  Json Entity::Save()
  {
    Entity* entity { this };

    Json entityJson;
    entityJson[ "mPosition" ].DeepCopy( Vector3ToJson( entity->mRelativeSpace.mPosition ) );
    entityJson[ "mScale" ].DeepCopy( Vector3ToJson( entity->mRelativeSpace.mScale ) );
    entityJson[ "mName" ].SetString( entity->mName );
    entityJson[ "mEulerRads" ].DeepCopy( Vector3ToJson( entity->mRelativeSpace.mEulerRads ) );
    entityJson[ "mActive" ].SetBool( entity->mActive );
    entityJson[ "mStatic" ].SetBool( entity->mStatic );
    // mEntityUUID is not saved, it is runtime only.

    for( Component* component : entity->mComponents )
    {
      const ComponentInfo* entry { component->GetEntry() };
      Json componentJson;
      entry->mMetaType->JsonSerialize( &componentJson, component );

      entityJson[ StringView( entry->mName ) ].DeepCopy( &componentJson );
    }

    if( !entity->mChildren.empty() )
    {
      Json& childrenJson { entityJson[ "mChildren" ] };
      for( Entity* childEntity : entity->mChildren )
        childrenJson.AddChild( childEntity->Save() );
    }

    return entityJson;
  }

  void Entity::Load( EntityUUIDCounter& uuidCounter, const Json& prefabJson )
  {
    TAC_ASSERT_MSG( mEntityUUID != NullEntityUUID,
                    "This entity has not been spawned, and is therefore missing an ID" );
    Json* jsonPos { prefabJson.FindChild( "mPosition" ) };
    Json* jsonScale { prefabJson.FindChild( "mScale" ) };
    Json* jsonEuler { prefabJson.FindChild( "mEulerRads" ) };
    Json* jsonName { prefabJson.FindChild( "mName" ) };
    Json* jsonActive { prefabJson.FindChild( "mActive" ) };
    Json* jsonStatic { prefabJson.FindChild( "mStatic" ) };

    Entity* entity { this };
    entity->mRelativeSpace.mPosition = Vector3FromJson( *jsonPos );
    entity->mRelativeSpace.mScale = Vector3FromJson( *jsonScale );
    entity->mRelativeSpace.mEulerRads = Vector3FromJson( *jsonEuler );
    entity->mName = jsonName->mString;
    entity->mActive = jsonActive ? jsonActive->mBoolean : entity->mActive;
    entity->mStatic = jsonStatic ? jsonStatic->mBoolean : entity->mStatic;

    // I think these should have its own mComponents json node
    // ^ 2025-11-11 totally agree, it's gross as it is.
    //              You'd have to change all the .prefab files though
    for( auto& [key, componentJson] : prefabJson.mObjectChildrenMap )
    {
      if( ComponentInfo * componentInfo{ ComponentInfo::Find( key.c_str() ) } )
      {
        Component* component{ entity->AddNewComponent( componentInfo ) };
        componentInfo->mMetaType->JsonDeserialize( componentJson, component );
      }
    }


    if( auto childrenJson{ prefabJson.mObjectChildrenMap.find( "mChildren" ) };
        childrenJson != prefabJson.mObjectChildrenMap.end() )
    {
      for( auto& [_, arrayElements] { ( *childrenJson ) };
           Json* childJson : arrayElements->mArrayElements )
      {
        Entity* childEntity { mWorld->SpawnEntity( uuidCounter.AllocateNewUUID() ) };
        childEntity->Load( uuidCounter, *childJson );
        entity->AddChild( childEntity );
      }
    }
  }

} // namespace Tac

