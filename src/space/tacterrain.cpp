#include "space/tacterrain.h"
#include "space/tacphysics.h"
#include "space/tacentity.h"

TacComponentRegistryEntry* TacTerrain::ComponentRegistryEntry = []()
{
  TacComponentRegistryEntry* entry = TacComponentRegistry::Instance()->RegisterNewEntry();
  entry->mName = "Terrain";
  entry->mCreateFn = []( TacWorld* world ) -> TacComponent*
  {
    return TacPhysics::GetSystem( world )->CreateTerrain();
  };
  entry->mDestroyFn = []( TacWorld* world, TacComponent* component )
  {
    TacPhysics::GetSystem( world )->DestroyTerrain( ( TacTerrain* )component );
  };
  entry->mNetworkBits = {};
  //entry->mSystemRegistryEntry = TacPhysics::SystemRegistryEntry;
  return entry;
}( );
TacComponentRegistryEntry* TacTerrain::GetEntry() { return TacTerrain::ComponentRegistryEntry; }
TacTerrain* TacTerrain::GetComponent( TacEntity* entity )
{
  return ( TacTerrain* )entity->GetComponent( TacTerrain::ComponentRegistryEntry );
}
