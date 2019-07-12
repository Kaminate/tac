#include "space/tacterrain.h"
#include "space/tacphysics.h"
#include "space/tacentity.h"

TacComponentRegistryEntry* TacTerrain::GetEntry() { return TacTerrain::ComponentRegistryEntry; }
TacTerrain* TacTerrain::GetComponent( TacEntity* entity )
{
  return ( TacTerrain* )entity->GetComponent( TacTerrain::ComponentRegistryEntry );
}
TacComponentRegistryEntry* TacTerrain::GetComponentRegistryEntry()
{
  return TacTerrain::ComponentRegistryEntry;
}
