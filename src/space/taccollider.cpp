#include "taccollider.h"
#include "tacentity.h"
#include "tacphysics.h"
void TacCollider::TacDebugImgui()
{
  //if( !ImGui::CollapsingHeader( "Collider" ) )
  //  return;
  //ImGui::Indent();
  //OnDestruct( ImGui::Unindent() );
  //ImGui::DragFloat3( "Velocity" , mVelocity.data(), 0.1f );
  //ImGui::DragFloat( "Capsule Radius", &mRadius, 0.1f );
  //ImGui::DragFloat( "Capsule Height", &mTotalHeight, 0.1f );
}

TacCollider* TacCollider::GetCollider( TacEntity* entity )
{
  return ( TacCollider* )entity->GetComponent( TacCollider::ComponentRegistryEntry );
}

TacComponentRegistryEntry* TacCollider::ComponentRegistryEntry = []()
{
  TacComponentRegistryEntry* entry = TacComponentRegistry::Instance()->RegisterNewEntry();
  entry->mName = "Collider";
  entry->mSystemRegistryEntry = TacPhysics::SystemRegistryEntry;
  entry->mNetworkBits = TacColliderBits;
  entry->mCreateFn = []( TacWorld* world ) -> TacComponent*
  {
    return TacPhysics::GetSystem( world )->CreateCollider();
  };
  return entry;
}( );
TacComponentRegistryEntry* TacCollider::GetEntry()
{
  return TacCollider::ComponentRegistryEntry;
}
