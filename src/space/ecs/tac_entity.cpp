#include "space/ecs/tac_component.h"
#include "space/ecs/tac_entity.h"
#include "space/ecs/tac_system.h"
#include "space/world/tac_world.h"
#include "src/common/memory/tac_frame_memory.h"
#include "src/common/algorithm/tac_algorithm.h"
#include "src/common/dataprocess/tac_json.h"
#include "src/common/preprocess/tac_preprocessor.h"

//#include <algorithm>
//#include <cmath>

namespace Tac
{
  struct ComponentFindFunctor
  {
    bool operator()( Component* component ) { return component->GetEntry() == componentRegistryEntry; }
    const ComponentRegistryEntry* componentRegistryEntry;
  };

  static v3 Vector3FromJson( Json& json )
  {
    v3 v =
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

  RelativeSpace RelativeSpaceFromMatrix( const m4& mLocal )
  {
    v3 c0 = { mLocal.m00, mLocal.m10, mLocal.m20 };
    v3 c1 = { mLocal.m01, mLocal.m11, mLocal.m21 };
    v3 c2 = { mLocal.m02, mLocal.m12, mLocal.m22 };
    v3 c3 = { mLocal.m03, mLocal.m13, mLocal.m23 };
    v3 scale = { Length( c0 ), Length( c1 ), Length( c2 ) };
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
    float zPhi = 0;
    float yTheta = 0;
    float xPsi = 0;


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

  void                      Components::Add( Component* component )
  {
    mComponents.push_back( component );
  }

  void                      Components::Clear()
  {
    mComponents.clear();
  }

  Component*                Components::Remove( const ComponentRegistryEntry* componentRegistryEntry )
  {
    auto it = std::find_if( mComponents.begin(),
                            mComponents.end(),
                            ComponentFindFunctor{ componentRegistryEntry } );
    TAC_ASSERT( it != mComponents.end() );
    Component* component = *it;
    mComponents.erase( it );
    return component;
  }

  //Components::ConstIterator Components::begin() const { return mComponents.begin(); }

  //Components::ConstIterator Components::end() const { return mComponents.end(); }

  Entity::~Entity()
  {
    RemoveAllComponents();
    // Assert that the world no longer contains this entity?
  }

  void             Entity::RemoveAllComponents()
  {
    for( Component* component : mComponents )
    {
      const ComponentRegistryEntry* entry = component->GetEntry();
      entry->mDestroyFn( mWorld, component );
    }
    mComponents.Clear();
  }

  Component*       Entity::AddNewComponent( const ComponentRegistryEntry* entry )
  {
    TAC_ASSERT( entry );
    TAC_ASSERT( !HasComponent( entry ) );
    TAC_ASSERT( entry->mCreateFn );
    Component* component = entry->mCreateFn( mWorld );
    TAC_ASSERT( component );
    mComponents.Add( component );
    component->mEntity = this;
    return component;
  }

  Component*       Entity::GetComponent( const ComponentRegistryEntry* entry )
  {
    for( Component* component : mComponents )
      if( component->GetEntry() == entry )
        return component;
    return nullptr;
  }

  const Component* Entity::GetComponent( const ComponentRegistryEntry* entry ) const
  {
    for( const Component* component : mComponents )
      if( component->GetEntry() == entry )
        return component;
    return nullptr;
  }

  bool             Entity::HasComponent( const ComponentRegistryEntry* entry )
  {
    return GetComponent( entry ) != nullptr;
  }

  void             Entity::RemoveComponent( const ComponentRegistryEntry* entry )
  {
    //TAC_ASSERT_UNIMPLEMENTED;
    //auto it = std::find_if(
    //	mComponents.begin(),
    //	mComponents.end(),
    //	[ & ]( Component* component ) { return component->GetEntry() == entry; } );
    //TAC_ASSERT( it != mComponents.end() );
    //Component* component = *it;
    //mComponents.erase( it );

    Component* component = mComponents.Remove( entry );

    //ComponentRegistryEntry* entry = component->GetEntry();
    entry->mDestroyFn( mWorld, component );
  }

  void             Entity::DeepCopy( const Entity& entity )
  {
    //TAC_ASSERT( mWorld && entity.mWorld && mWorld != entity.mWorld );
    mEntityUUID = entity.mEntityUUID;
    mName = entity.mName;
    mInheritParentScale = entity.mInheritParentScale;
    mRelativeSpace = entity.mRelativeSpace;
    RemoveAllComponents();

    //for( auto oldComponent : entity.mComponents )
    //{
    //  auto componentType = oldComponent->GetComponentType();
    //  auto newComponent = AddNewComponent( componentType );
    //  auto componentData = GetComponentData( componentType );
    //  for( auto& networkBit : componentData->mNetworkBits )
    //  {
    //    auto dst = ( char* )newComponent + networkBit.mByteOffset;
    //    auto src = ( char* )oldComponent + networkBit.mByteOffset;
    //    auto size = networkBit.mComponentByteCount * networkBit.mComponentCount;
    //    std::memcpy( dst, src, size );
    //  }
    //}
    for( Entity* child : mChildren )
      mWorld->KillEntity( child );
    mChildren.clear();
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
#if 0
    ImGui::PushID( this );
    OnDestruct( ImGui::PopID() );

    const auto headerName = ShortFixedString::Concant( "Entity id ", ToString( mEntityUUID );
    if( !ImGui::CollapsingHeader( headerName, ImGuiTreeNodeFlags_DefaultOpen ) )
      return;

    ImGui::Indent();
    OnDestruct( ImGui::Unindent() );
    ImGui::DragFloat3( "Position", mPosition.data(), 0.1f );

    Vector< ComponentRegistryEntryIndex > couldBeAdded;
    for( int i = 0; i < ( int )ComponentRegistryEntryIndex::Count; i++ )
    {
      auto componentType = ( ComponentRegistryEntryIndex )i;
      if( HasComponent( componentType ) )
        continue;

      auto componentData = GetComponentData( componentType );
      if( !componentData )
        continue;
      couldBeAdded.push_back( componentType );
    }

    if( !couldBeAdded.empty() && ImGui::BeginMenu( "Add Component" ) )
    {
      for( ComponentRegistryEntryIndex componentType : couldBeAdded )
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
    for( int iChild = 0; iChild < mParent->mChildren.size(); ++iChild )
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

  void             Entity::Save( Json& entityJson )
  {
    Entity* entity = this;
    entityJson[ "mPosition" ].DeepCopy( Vector3ToJson( entity->mRelativeSpace.mPosition ) );
    entityJson[ "mScale" ].DeepCopy( Vector3ToJson( entity->mRelativeSpace.mScale ) );
    entityJson[ "mName" ].SetString( entity->mName );
    entityJson[ "mEulerRads" ].DeepCopy( Vector3ToJson( entity->mRelativeSpace.mEulerRads ) );
    entityJson[ "mEntityUUID" ].SetNumber( ( JsonNumber )entity->mEntityUUID );
    entityJson[ "mActive" ].SetBool( entity->mActive );

    for( Component* component : entity->mComponents )
    {
      auto entry = component->GetEntry();
      Json componentJson;
      if( entry->mSaveFn )
        entry->mSaveFn( componentJson, component );
      entityJson[ StringView( entry->mName ) ].DeepCopy( &componentJson );
    }

    if( !entity->mChildren.empty() )
    {
      Json& childrenJson = entityJson[ "mChildren" ];
      for( Entity* childEntity : entity->mChildren )
        childEntity->Save( *childrenJson.AddChild() );
    }
  }

  void             Entity::Load( Json& prefabJson )
  {
    Json* jsonPos = prefabJson.FindChild( "mPosition" );
    Json* jsonScale = prefabJson.FindChild( "mScale" );
    Json* jsonEuler = prefabJson.FindChild( "mEulerRads" );
    Json* jsonName = prefabJson.FindChild( "mName" );
    Json* jsonUUID = prefabJson.FindChild( "mEntityUUID" );
    Json* jsonActive = prefabJson.FindChild( "mActive" );
    Entity* entity = this;
    entity->mRelativeSpace.mPosition = Vector3FromJson( *jsonPos );
    entity->mRelativeSpace.mScale = Vector3FromJson( *jsonScale );
    entity->mRelativeSpace.mEulerRads = Vector3FromJson( *jsonEuler );
    entity->mName = jsonName->mString;
    entity->mEntityUUID = ( EntityUUID )( UUID )jsonUUID->mNumber;
    entity->mActive = jsonActive ? jsonActive->mBoolean : entity->mActive;

    // I think these should have its own mComponents json node
    for( auto& pair : prefabJson.mObjectChildrenMap )
    {
      StringView key = pair.first;
      Json* componentJson = pair.second;
      ComponentRegistryEntry* componentRegistryEntry = ComponentRegistry_FindComponentByName( key );
      if( !componentRegistryEntry )
        continue; // This key-value pair is not a component

      TAC_ASSERT( componentRegistryEntry );
      Component* component = entity->AddNewComponent( componentRegistryEntry );
      if( componentRegistryEntry->mLoadFn )
        componentRegistryEntry->mLoadFn( *componentJson, component );
    }

    if( Json* childrenJson = prefabJson.mObjectChildrenMap[ "mChildren" ] )
    {
      for( Json* childJson : childrenJson->mArrayElements )
      {
        Entity* childEntity = mWorld->SpawnEntity( NullEntityUUID );
        childEntity->Load( *childJson );

        entity->AddChild( childEntity );
      }
    }
  }

}

