#include "src/space/collider/taccollider.h"
#include "src/space/tacentity.h"
#include "src/space/physics/tacphysics.h"
namespace Tac
{


Collider* Collider::GetCollider( Entity* entity )
{
  return ( Collider* )entity->GetComponent( Collider::ColliderComponentRegistryEntry );
}

ComponentRegistryEntry* Collider::GetEntry()
{
  return Collider::ColliderComponentRegistryEntry;
}

static Component* CreateColliderComponent( World* world )
{
  return Physics::GetSystem( world )->CreateCollider();
}

static void DestroyColliderComponent( World* world, Component* component )
{
  Physics::GetSystem( world )->DestroyCollider( ( Collider* )component );
}
ComponentRegistryEntry* Collider::ColliderComponentRegistryEntry;

void ColliderDebugImgui( Component* );
void Collider::SpaceInitPhysicsCollider()
{
  Collider::ColliderComponentRegistryEntry = ComponentRegistry_RegisterComponent();
  Collider::ColliderComponentRegistryEntry->mName = "Collider";
  Collider::ColliderComponentRegistryEntry->mNetworkBits = ColliderBits;
  Collider::ColliderComponentRegistryEntry->mCreateFn = CreateColliderComponent;
  Collider::ColliderComponentRegistryEntry->mDestroyFn = DestroyColliderComponent;
  Collider::ColliderComponentRegistryEntry->mDebugImguiFn = ColliderDebugImgui;
}

}

