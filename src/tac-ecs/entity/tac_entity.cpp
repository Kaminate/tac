#include "tac_entity.h" // self-inc

#include "tac-ecs/component/tac_component.h"
#include "tac-ecs/component/tac_component_registry.h"
#include "tac-ecs/system/tac_system.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/dataprocess/tac_json.h"
//#include "tac-std-lib/memory/tac_frame_memory.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/meta/tac_meta_composite.h"
#include "tac-std-lib/meta/tac_meta.h"

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


  //struct MetaEntity : public MetaCompositeType
  //{
  //  MetaEntity() : MetaCompositeType( "Entity", sizeof( Entity ), {} )
  //  {
  //    mMetaVars.push_back( TAC_META_MEMBER( Entity, mEntityUUID ) );
  //    mMetaVars.push_back( TAC_META_MEMBER( Entity, mEntityUUID ) );
  //  }
  //};

}

#if 0
Tac::RelativeSpace Tac::RelativeSpaceFromMatrix( const m4& mLocal )
  {
    v3 c0  { mLocal.m00, mLocal.m10, mLocal.m20 };
    v3 c1  { mLocal.m01, mLocal.m11, mLocal.m21 };
    v3 c2  { mLocal.m02, mLocal.m12, mLocal.m22 };
    v3 c3  { mLocal.m03, mLocal.m13, mLocal.m23 };
    v3 scale  { Length( c0 ), Length( c1 ), Length( c2 ) };
    c0 /= scale.x;
    c1 /= scale.y;
    c2 /= scale.z;

    float
      r11, r12, r13,
      r21, r22, r23,
      r31, r32, r33;
    {
      r11 = c0[ 0 ];
      r21 = c0[ 1 ];
      r31 = c0[ 2 ];

      r12 = c1[ 0 ];
      r22 = c1[ 1 ];
      r32 = c1[ 2 ];

      r13 = c2[ 0 ];
      r23 = c2[ 1 ];
      r33 = c2[ 2 ];
    }

    TAC_UNUSED_PARAMETER( r22 );
    TAC_UNUSED_PARAMETER( r23 );

    // On-rotation-deformation-zones-for-finite-strain-Cosserat-plasticity.pdf
    // Rot( x, y, z ) = rotZ(phi) * rotY(theta) * rotX(psi)
    float zPhi {};
    float yTheta {};
    float xPsi {};


    if( r31 != 1.0f && r31 != -1.0f )
    {
      yTheta = -Asin( r31 );
      xPsi = Atan2( r32 / Cos( yTheta ), r33 / Cos( yTheta ) );
      zPhi = Atan2( r21 / Cos( yTheta ), r11 / Cos( yTheta ) );
    }
    else
    {
      zPhi = 0;
      if( r31 == -1.0f )
      {
        yTheta = 3.14f / 2.0f;
        xPsi = Atan2( r12, r13 );
      }
      else
      {
        yTheta = -3.14f / 2.0f;
        xPsi = Atan2( -r12, -r13 );
      }
    }

    return { .mPosition = c3, .mEulerRads = { xPsi, yTheta, zPhi }, .mScale = scale };
  }
#endif

namespace Tac
{
  // -----------------------------------------------------------------------------------------------

  Entity::~Entity()
  {
    RemoveAllComponents();
    // Assert that the world no longer contains this entity?
  }

  void             Entity::RemoveAllComponents()
  {
    for( Component* component : mComponents )
    {
      const ComponentInfo* entry { component->GetEntry() };
      entry->mDestroyFn( mWorld, component );
    }
    mComponents.Clear();
  }

  Component*       Entity::AddNewComponent( const ComponentInfo* entry )
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

  dynmc Component* Entity::GetComponent( const ComponentInfo* entry ) dynmc
  {
    for( Component* component : mComponents )
      if( component->GetEntry() == entry )
        return component;
    return nullptr;
  }

  const Component* Entity::GetComponent( const ComponentInfo* entry ) const
  {
    for( const Component* component : mComponents )
      if( component->GetEntry() == entry )
        return component;
    return nullptr;
  }

  bool             Entity::HasComponent( const ComponentInfo* entry ) const
  {
    return GetComponent( entry ) != nullptr;
  }

  void             Entity::RemoveComponent( const ComponentInfo* entry )
  {
    Component* component { mComponents.Remove( entry ) };
    entry->mDestroyFn( mWorld, component );
  }

  void             Entity::DeepCopy( const Entity& entity )
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

  void             Entity::AddChild( Entity* child )
  {
    TAC_ASSERT( !Contains( mChildren, child ) );
    TAC_ASSERT( child->mParent != this );
    child->Unparent();
    child->mParent = this;
    mChildren.push_back( child );
  }

  void             Entity::DebugImgui()
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

  void             Entity::Unparent()
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

  Json             Entity::Save()
  {
    Entity* entity { this };

    Json entityJson;
    entityJson[ "mPosition" ].DeepCopy( Vector3ToJson( entity->mRelativeSpace.mPosition ) );
    entityJson[ "mScale" ].DeepCopy( Vector3ToJson( entity->mRelativeSpace.mScale ) );
    entityJson[ "mName" ].SetString( entity->mName );
    entityJson[ "mEulerRads" ].DeepCopy( Vector3ToJson( entity->mRelativeSpace.mEulerRads ) );
    entityJson[ "mEntityUUID" ].SetNumber( ( JsonNumber )entity->mEntityUUID );
    entityJson[ "mActive" ].SetBool( entity->mActive );
    entityJson[ "mStatic" ].SetBool( entity->mStatic );

    for( Component* component : entity->mComponents )
    {
      const ComponentInfo* entry { component->GetEntry() };
      Json componentJson;
      entry->mMetaType->JsonSerialize( &componentJson, component );
      //if( entry->mSaveFn )
      //  entry->mSaveFn( componentJson, component );

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

  void             Entity::Load( const Json& prefabJson )
  {
    Json* jsonPos { prefabJson.FindChild( "mPosition" ) };
    Json* jsonScale { prefabJson.FindChild( "mScale" ) };
    Json* jsonEuler { prefabJson.FindChild( "mEulerRads" ) };
    Json* jsonName { prefabJson.FindChild( "mName" ) };
    Json* jsonUUID { prefabJson.FindChild( "mEntityUUID" ) };
    Json* jsonActive { prefabJson.FindChild( "mActive" ) };
    Json* jsonStatic { prefabJson.FindChild( "mStatic" ) };

    Entity* entity { this };
    entity->mRelativeSpace.mPosition = Vector3FromJson( *jsonPos );
    entity->mRelativeSpace.mScale = Vector3FromJson( *jsonScale );
    entity->mRelativeSpace.mEulerRads = Vector3FromJson( *jsonEuler );
    entity->mName = jsonName->mString;
    entity->mEntityUUID = ( EntityUUID )( UUID )jsonUUID->mNumber;
    entity->mActive = jsonActive ? jsonActive->mBoolean : entity->mActive;
    entity->mStatic = jsonStatic ? jsonStatic->mBoolean : entity->mStatic;

    // I think these should have its own mComponents json node
    for( auto& [key, componentJson] : prefabJson.mObjectChildrenMap )
    {
      //const StringView key { pair.mFirst };
      //Json* componentJson { pair.mSecond };

      ComponentInfo* componentInfo { ComponentInfo::Find( key.c_str() ) };
      if( !componentInfo )
        continue; // This key-value pair is not a component

      TAC_ASSERT( componentInfo );
      Component* component { entity->AddNewComponent( componentInfo ) };
      //if( componentInfo->mLoadFn )
      //  componentInfo->mLoadFn( *componentJson, component );
      componentInfo->mMetaType->JsonDeserialize( componentJson, component );
    }


    if( auto childrenJson{ prefabJson.mObjectChildrenMap.find( "mChildren" ) };
        childrenJson != prefabJson.mObjectChildrenMap.end() )
    {
      auto& [_, arrayElements] { ( *childrenJson ) };
      for( Json* childJson : arrayElements->mArrayElements )
      {
        Entity* childEntity { mWorld->SpawnEntity( NullEntityUUID ) };
        childEntity->Load( *childJson );

        entity->AddChild( childEntity );
      }
    }
  }

} // namespace Tac

