//#include "space/graphics/tacgraphics.h"
#include "space/taccomponent.h"
#include "space/tacentity.h"
#include "space/tacsystem.h"
#include "space/tacworld.h"

#include "common/tacAlgorithm.h"
#include "common/tacJson.h"
#include "common/tacPreprocessor.h"

#include <algorithm>

//#include <cstring> // used for what?


TacEntity::~TacEntity()
{
  RemoveAllComponents();
  // Assert that the world no longer contains this entity?
}

void TacEntity::RemoveAllComponents()
{
  for( TacComponent* component : mComponents )
  {
    TacComponentRegistryEntry* entry = component->GetEntry();
    entry->mDestroyFn( mWorld, component );
  }
  mComponents.clear();
}

TacComponent* TacEntity::AddNewComponent( TacComponentRegistryEntry* entry )
{
  TacAssert( entry );
  TacAssert( !HasComponent( entry ) );
  TacAssert( entry->mCreateFn );
  TacComponent* component = entry->mCreateFn( mWorld );
  TacAssert( component );
  mComponents.push_back( component );
  component->mEntity = this;
  return component;
}
TacComponent* TacEntity::GetComponent( TacComponentRegistryEntry* entry )
{
  for( TacComponent* component : mComponents )
    if( component->GetEntry() == entry )
      return component;
  return nullptr;
}
const TacComponent* TacEntity::GetComponent( TacComponentRegistryEntry* entry ) const
{
  for( auto component : mComponents )
    if( component->GetEntry() == entry )
      return component;
  return nullptr;
}

bool TacEntity::HasComponent( TacComponentRegistryEntry* entry )
{
  return GetComponent( entry ) != nullptr;
}

void TacEntity::RemoveComponent( TacComponentRegistryEntry* entry )
{
  TacUnimplemented;
  auto it = std::find_if(
    mComponents.begin(),
    mComponents.end(),
    [ & ]( TacComponent* component ) { return component->GetEntry() == entry; } );
  TacAssert( it != mComponents.end() );
  TacComponent* component = *it;
  mComponents.erase( it );
  //auto componentData = TacGetComponentData( componentType );
  //auto system = mWorld->GetSystem( componentData->mSystemType );
  //// todo:
  ////   break this up  into a thing
  ////   that auto does it when you register each system
  //auto graphics = ( TacGraphics* )mWorld->GetSystem( TacSystemType::Graphics );
  //switch( componentType )
  //{
  //  case TacComponentRegistryEntryIndex::Say:
  //    break;
  //  case TacComponentRegistryEntryIndex::Model:
  //    graphics->DestroyModelComponent( ( TacModel* )component );
  //    break;
  //  case TacComponentRegistryEntryIndex::Collider:
  //    break;
  //  case TacComponentRegistryEntryIndex::Terrain:
  //    break;
  //    TacInvalidDefaultCase( componentType );
  //}

}


void TacEntity::DeepCopy( const TacEntity& entity )
{
  TacAssert( mWorld && entity.mWorld && mWorld != entity.mWorld );
  mEntityUUID = entity.mEntityUUID;
  RemoveAllComponents();
  //for( auto oldComponent : entity.mComponents )
  //{
  //  auto componentType = oldComponent->GetComponentType();
  //  auto newComponent = AddNewComponent( componentType );
  //  auto componentData = TacGetComponentData( componentType );
  //  for( auto& networkBit : componentData->mNetworkBits )
  //  {
  //    auto dst = ( char* )newComponent + networkBit.mByteOffset;
  //    auto src = ( char* )oldComponent + networkBit.mByteOffset;
  //    auto size = networkBit.mComponentByteCount * networkBit.mComponentCount;
  //    std::memcpy( dst, src, size );
  //  }
  //}
}

void TacEntity::AddChild( TacEntity* child )
{
  TacAssert( !TacContains( mChildren, child ) );
  TacAssert( child->mParent != this );
  child->Unparent();
  child->mParent = this;
  mChildren.push_back( child );
}
void TacEntity::TacDebugImgui()
{
  #if 0
  ImGui::PushID( this );
  OnDestruct( ImGui::PopID() );
  if( !ImGui::CollapsingHeader( va( "Entity id %i", mEntityUUID ), ImGuiTreeNodeFlags_DefaultOpen ) )
    return;
  ImGui::Indent();
  OnDestruct( ImGui::Unindent() );
  ImGui::DragFloat3( "Position", mPosition.data(), 0.1f );

  TacVector< TacComponentRegistryEntryIndex > couldBeAdded;
  for( int i = 0; i < ( int )TacComponentRegistryEntryIndex::Count; i++ )
  {
    auto componentType = ( TacComponentRegistryEntryIndex )i;
    if( HasComponent( componentType ) )
      continue;
    auto componentData = TacGetComponentData( componentType );
    if( !componentData )
      continue;
    couldBeAdded.push_back( componentType );
  }
  if( !couldBeAdded.empty() && ImGui::BeginMenu( "Add Component" ) )
  {
    for( auto componentType : couldBeAdded )
    {
      auto componentData = TacGetComponentData( componentType );
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
      auto componentData = TacGetComponentData( componentType );
      if( !ImGui::MenuItem( componentData->mName ) )
        continue;
      RemoveComponent( componentType );
      break;
    }
    ImGui::EndMenu();
  }
  for( auto component : mComponents )
    component->TacDebugImgui();
  #endif
}

void TacEntity::Unparent()
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

void TacEntity::Save( TacJson& entityJson )
{
  TacEntity* entity = this;
  TacJson posJson;
  {
    v3 position = entity->mLocalPosition;
    posJson[ "x" ] = position.x;
    posJson[ "y" ] = position.y;
    posJson[ "z" ] = position.z;
  }

  TacJson scaleJson;
  {
    v3 scale = entity->mLocalScale;
    scaleJson[ "x" ] = scale.x;
    scaleJson[ "y" ] = scale.y;
    scaleJson[ "z" ] = scale.z;
  }

  TacJson eulerRadsJson;
  {
    eulerRadsJson[ "x" ] = entity->mLocalEulerRads.x;
    eulerRadsJson[ "y" ] = entity->mLocalEulerRads.y;
    eulerRadsJson[ "z" ] = entity->mLocalEulerRads.z;
  }
  entityJson[ "mPosition" ] = posJson;
  entityJson[ "mScale" ] = scaleJson;
  entityJson[ "mName" ] = entity->mName;
  entityJson[ "mEulerRads" ] = eulerRadsJson;
  entityJson[ "mEntityUUID" ] = ( TacJsonNumber )entity->mEntityUUID;

  for( TacComponent* component : entity->mComponents )
  {
    auto entry = component->GetEntry();
    TacJson componentJson;
    if( entry->mSaveFn )
      entry->mSaveFn( componentJson, component );
    entityJson[ entry->mName ] = componentJson;
  }

  if( !entity->mChildren.empty() )
  {
    TacJson childrenJson;
    childrenJson.mType = TacJsonType::Array;
    for( TacEntity* child : entity->mChildren )
    {
      auto childJson = new TacJson;
      child->Save( *childJson );
      childrenJson.mElements.push_back( childJson );
    }
    entityJson[ "mChildren" ] = childrenJson;
  }
}

void TacEntity::Load( TacJson& prefabJson )
{
  TacEntity* entity = this;


  TacJson& positionJson = prefabJson[ "mPosition" ];
  v3 pos =
  {
    ( float )positionJson[ "x" ].mNumber,
    ( float )positionJson[ "y" ].mNumber,
    ( float )positionJson[ "z" ].mNumber,
  };

  TacJson& scaleJson = prefabJson[ "mScale" ];
  v3 scale =
  {
    ( float )scaleJson[ "x" ].mNumber,
    ( float )scaleJson[ "y" ].mNumber,
    ( float )scaleJson[ "z" ].mNumber,
  };

  TacJson& eulerRadsJson = prefabJson[ "mEulerRads" ];
  v3 eulerRads =
  {
    ( float )eulerRadsJson[ "x" ].mNumber,
    ( float )eulerRadsJson[ "y" ].mNumber,
    ( float )eulerRadsJson[ "z" ].mNumber,
  };

  entity->mLocalPosition = pos;
  entity->mLocalScale = scale;
  entity->mLocalEulerRads = eulerRads;
  entity->mName = prefabJson[ "mName" ].mString;
  entity->mEntityUUID = ( TacEntityUUID )( TacUUID )prefabJson[ "mEntityUUID" ].mNumber;

  TacComponentRegistry* componentRegistry = TacComponentRegistry::Instance();
  // I think these should have its own mComponents json node
  for( auto& prefabJson : prefabJson.mChildren )
  {
    const TacString& key = prefabJson.first;
    TacJson* componentJson = prefabJson.second;
    TacComponentRegistryEntry* componentRegistryEntry = componentRegistry->FindEntryNamed( key );
    if( !componentRegistryEntry )
      continue; // This key-value pair is not a component

    TacAssert( componentRegistryEntry );
    TacComponent* component = entity->AddNewComponent( componentRegistryEntry );
    if( componentRegistryEntry->mLoadFn )
      componentRegistryEntry->mLoadFn( *componentJson, component );
  }

  if( TacJson* childrenJson = prefabJson.mChildren[ "mChildren" ] )
  {
    for( TacJson* childJson : childrenJson->mElements )
    {
      TacEntity* childEntity = mWorld->SpawnEntity( TacNullEntityUUID );
      childEntity->Load( *childJson );

      entity->AddChild( childEntity );
    }
  }
}
