#include "space/tacentity.h"
#include "space/taccomponent.h"
#include "space/tacworld.h"
#include "space/tacsystem.h"
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
  for( auto component : mComponents )
  {
    auto componentData = TacGetComponentData( component->GetComponentType() );
    auto system = mWorld->GetSystem( componentData->mSystemType );
    system->DestroyComponent( component );
  }
  mComponents.clear();
}

TacComponent* TacEntity::AddNewComponent( TacComponentType componentType )
{
  TacAssert( !HasComponent( componentType ) );
  auto componentData = TacGetComponentData( componentType );
  auto system = mWorld->GetSystem( componentData->mSystemType );
  auto component = system->CreateComponent( componentType );
  mComponents.push_back( component );
  component->mEntity = this;
  return component;
}

TacComponent* TacEntity::GetComponent( TacComponentType type )
{
  for( auto component : mComponents )
    if( component->GetComponentType() == type )
      return component;
  return nullptr;
}

bool TacEntity::HasComponent( TacComponentType componentType )
{
  return GetComponent( componentType ) != nullptr;
}

void TacEntity::RemoveComponent( TacComponentType type )
{
  auto it = std::find_if(
    mComponents.begin(),
    mComponents.end(),
    [ & ]( TacComponent* component ) { return component->GetComponentType() == type; } );
  TacAssert( it != mComponents.end() );
  auto component = *it;
  mComponents.erase( it );
  auto componentData = TacGetComponentData( type );
  auto system = mWorld->GetSystem( componentData->mSystemType );
  system->DestroyComponent( component );
}

void TacEntity::DeepCopy( const TacEntity& entity )
{
  TacAssert( mWorld && entity.mWorld && mWorld != entity.mWorld );
  mEntityUUID = entity.mEntityUUID;
  RemoveAllComponents();
  for( auto oldComponent : entity.mComponents )
  {
    auto componentType = oldComponent->GetComponentType();
    auto newComponent = AddNewComponent( componentType );
    auto componentData = TacGetComponentData( componentType );
    for( auto& networkBit : componentData->mNetworkBits )
    {
      auto dst = ( char* )newComponent + networkBit.mByteOffset;
      auto src = ( char* )oldComponent + networkBit.mByteOffset;
      auto size = networkBit.mComponentByteCount * networkBit.mComponentCount;
      std::memcpy( dst, src, size );
    }
  }
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

  TacVector< TacComponentType > couldBeAdded;
  for( int i = 0; i < ( int )TacComponentType::Count; i++ )
  {
    auto componentType = ( TacComponentType )i;
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

