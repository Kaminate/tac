#include "space/collider/taccollider.h"
#include "space/tacentity.h"
#include "space/physics/tacphysics.h"


TacCollider* TacCollider::GetCollider( TacEntity* entity )
{
  return ( TacCollider* )entity->GetComponent( TacCollider::ColliderComponentRegistryEntry );
}

TacComponentRegistryEntry* TacCollider::GetEntry()
{
  return TacCollider::ColliderComponentRegistryEntry;
}

static TacComponent* TacCreateColliderComponent( TacWorld* world )
{
  return TacPhysics::GetSystem( world )->CreateCollider();
}

static void TacDestroyColliderComponent( TacWorld* world, TacComponent* component )
{
  TacPhysics::GetSystem( world )->DestroyCollider( ( TacCollider* )component );
}
TacComponentRegistryEntry* TacCollider::ColliderComponentRegistryEntry;

void TacColliderDebugImgui( TacComponent* );
void TacCollider::TacSpaceInitPhysicsCollider()
{
  TacCollider::ColliderComponentRegistryEntry = TacComponentRegistry::Instance()->RegisterNewEntry();
  TacCollider::ColliderComponentRegistryEntry->mName = "Collider";
  TacCollider::ColliderComponentRegistryEntry->mNetworkBits = TacColliderBits;
  TacCollider::ColliderComponentRegistryEntry->mCreateFn = TacCreateColliderComponent;
  TacCollider::ColliderComponentRegistryEntry->mDestroyFn = TacDestroyColliderComponent;
  TacCollider::ColliderComponentRegistryEntry->mDebugImguiFn = TacColliderDebugImgui;
}
