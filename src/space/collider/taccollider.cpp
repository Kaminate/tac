#include "space/collider/taccollider.h"
#include "space/tacentity.h"
#include "space/physics/tacphysics.h"


TacCollider* TacCollider::GetCollider( TacEntity* entity )
{
  return ( TacCollider* )entity->GetComponent( TacCollider::ComponentRegistryEntry );
}

TacComponentRegistryEntry* TacCollider::GetEntry()
{
  return TacCollider::ComponentRegistryEntry;
}
