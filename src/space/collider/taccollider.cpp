#include "taccollider.h"
#include "tacentity.h"
#include "tacphysics.h"


TacCollider* TacCollider::GetCollider( TacEntity* entity )
{
  return ( TacCollider* )entity->GetComponent( TacCollider::ComponentRegistryEntry );
}

TacComponentRegistryEntry* TacCollider::GetEntry()
{
  return TacCollider::ComponentRegistryEntry;
}
