#include "tacspace.h"

#include "tacgraphics.h"
#include "tacphysics.h"
#include "terrain/tacterrain.h"
#include "terrain/tacterraindebug.h"
#include "collider/taccollider.h"
#include "collider/taccolliderdebug.h"
#include "model/tacmodel.h"
#include "model/tacmodeldebug.h"

TacSystemRegistryEntry* TacGraphics::SystemRegistryEntry;
TacComponentRegistryEntry* TacModel::ComponentRegistryEntry;
static void TacSpaceInitGraphicsModel()
{
  TacModel::ComponentRegistryEntry = TacComponentRegistry::Instance()->RegisterNewEntry();
  TacModel::ComponentRegistryEntry->mName = "Model";
  TacModel::ComponentRegistryEntry->mNetworkBits = TacComponentModelBits;
  TacModel::ComponentRegistryEntry->mCreateFn = []( TacWorld* world )->TacComponent*
  {
    return TacGraphics::GetSystem( world )->CreateModelComponent();
  };
  TacModel::ComponentRegistryEntry->mDestroyFn = []( TacWorld* world, TacComponent* component )
  {
    TacGraphics::GetSystem( world )->DestroyModelComponent( ( TacModel* )component );
  };
  TacModel::ComponentRegistryEntry->mDebugImguiFn = TacModelDebugImgui;
}
static void TacSpaceInitGraphics()
{
  TacGraphics::SystemRegistryEntry = TacSystemRegistry::Instance()->RegisterNewEntry();
  TacGraphics::SystemRegistryEntry->mCreateFn = []() -> TacSystem* { return new TacGraphics; };
  TacSpaceInitGraphicsModel();


}

TacSystemRegistryEntry* TacPhysics::SystemRegistryEntry;
TacComponentRegistryEntry* TacTerrain::ComponentRegistryEntry;
TacComponentRegistryEntry* TacCollider::ComponentRegistryEntry;
static void TacSpaceInitPhysicsTerrain()
{
  TacTerrain::ComponentRegistryEntry = TacComponentRegistry::Instance()->RegisterNewEntry();
  TacTerrain::ComponentRegistryEntry->mName = "Terrain";
  TacTerrain::ComponentRegistryEntry->mCreateFn = []( TacWorld* world ) -> TacComponent*
  {
    return TacPhysics::GetSystem( world )->CreateTerrain();
  };
  TacTerrain::ComponentRegistryEntry->mDestroyFn = []( TacWorld* world, TacComponent* component )
  {
    TacPhysics::GetSystem( world )->DestroyTerrain( ( TacTerrain* )component );
  };
  TacTerrain::ComponentRegistryEntry->mNetworkBits = {};
  TacTerrain::ComponentRegistryEntry->mNetworkBits = {};
  TacTerrain::ComponentRegistryEntry->mDebugImguiFn = TacTerrainDebugImgui;
}
static void TacSpaceInitPhysicsCollider()
{
    TacCollider::ComponentRegistryEntry= TacComponentRegistry::Instance()->RegisterNewEntry();
    TacCollider::ComponentRegistryEntry->mName = "Collider";
    TacCollider::ComponentRegistryEntry->mNetworkBits = TacColliderBits;
    TacCollider::ComponentRegistryEntry->mCreateFn = []( TacWorld* world ) -> TacComponent*
    {
      return TacPhysics::GetSystem( world )->CreateCollider();
    };
    TacCollider::ComponentRegistryEntry->mDestroyFn = []( TacWorld* world, TacComponent* component )
    {
      TacPhysics::GetSystem( world )->DestroyCollider( ( TacCollider* )component );
    };
    TacCollider::ComponentRegistryEntry->mDebugImguiFn = TacColliderDebugImgui;
}
static void TacSpaceInitPhysics()
{
  TacPhysics::SystemRegistryEntry = TacSystemRegistry::Instance()->RegisterNewEntry();
  TacPhysics::SystemRegistryEntry->mCreateFn = []() -> TacSystem* { return new TacPhysics; };
  TacSpaceInitPhysicsTerrain();
  TacSpaceInitPhysicsCollider();

}

void TacSpaceInit()
{
  TacSpaceInitGraphics();
  TacSpaceInitPhysics();
}
