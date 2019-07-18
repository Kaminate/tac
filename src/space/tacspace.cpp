#include "space/tacspace.h"
#include "space/graphics/tacgraphics.h"
#include "space/graphics/tacgraphicsdebug.h"
#include "space/physics/tacphysics.h"
#include "space/physics/tacphysicsdebug.h"
#include "space/terrain/tacterrain.h"
#include "space/terrain/tacterraindebug.h"
#include "space/collider/taccollider.h"
#include "space/collider/taccolliderdebug.h"
#include "space/model/tacmodel.h"
#include "space/model/tacmodeldebug.h"

TacSystemRegistryEntry* TacGraphics::SystemRegistryEntry;
TacComponentRegistryEntry* TacModel::ComponentRegistryEntry;
static TacComponent* TacCreateModelComponent( TacWorld* world )
{
  return TacGraphics::GetSystem( world )->CreateModelComponent();
}
static void TacDestroyModelComponent( TacWorld* world, TacComponent* component )
{
  TacGraphics::GetSystem( world )->DestroyModelComponent( ( TacModel* )component );
}
static TacSystem* TacCreateGraphicsSystem()
{
  return new TacGraphics;
}
static void TacSpaceInitGraphicsModel()
{
  TacModel::ComponentRegistryEntry = TacComponentRegistry::Instance()->RegisterNewEntry();
  TacModel::ComponentRegistryEntry->mName = "Model";
  TacModel::ComponentRegistryEntry->mNetworkBits = TacComponentModelBits;
  TacModel::ComponentRegistryEntry->mCreateFn = TacCreateModelComponent;
  TacModel::ComponentRegistryEntry->mDestroyFn = TacDestroyModelComponent;
  TacModel::ComponentRegistryEntry->mDebugImguiFn = TacModelDebugImgui;
}
static void TacSpaceInitGraphics()
{
  TacGraphics::SystemRegistryEntry = TacSystemRegistry::Instance()->RegisterNewEntry();
  TacGraphics::SystemRegistryEntry->mCreateFn = TacCreateGraphicsSystem;
  TacGraphics::SystemRegistryEntry->mName = "Graphics";
  TacGraphics::SystemRegistryEntry->mDebugImGui = TacGraphicsDebugImgui;
  TacSpaceInitGraphicsModel();
}

TacSystemRegistryEntry* TacPhysics::SystemRegistryEntry;
TacComponentRegistryEntry* TacTerrain::ComponentRegistryEntry;
TacComponentRegistryEntry* TacCollider::ComponentRegistryEntry;
static TacSystem* TacCreatePhysicsSystem() { return new TacPhysics; }
static  TacComponent* TacCreateTerrainComponent( TacWorld* world )
{
  return TacPhysics::GetSystem( world )->CreateTerrain();
}
static  void TacDestroyTerrainComponent( TacWorld* world, TacComponent* component )
{
  TacPhysics::GetSystem( world )->DestroyTerrain( ( TacTerrain* )component );
};
static TacComponent* TacCreateColliderComponent( TacWorld* world )
{
  return TacPhysics::GetSystem( world )->CreateCollider();
}
static void TacDestroyColliderComponent( TacWorld* world, TacComponent* component )
{
  TacPhysics::GetSystem( world )->DestroyCollider( ( TacCollider* )component );
}
static void TacSpaceInitPhysicsTerrain()
{
  TacTerrain::ComponentRegistryEntry = TacComponentRegistry::Instance()->RegisterNewEntry();
  TacTerrain::ComponentRegistryEntry->mName = "Terrain";
  TacTerrain::ComponentRegistryEntry->mCreateFn = TacCreateTerrainComponent;
  TacTerrain::ComponentRegistryEntry->mDestroyFn = TacDestroyTerrainComponent;
  TacTerrain::ComponentRegistryEntry->mNetworkBits = {};
  TacTerrain::ComponentRegistryEntry->mNetworkBits = {};
  TacTerrain::ComponentRegistryEntry->mDebugImguiFn = TacTerrainDebugImgui;
}
static void TacSpaceInitPhysicsCollider()
{
  TacCollider::ComponentRegistryEntry = TacComponentRegistry::Instance()->RegisterNewEntry();
  TacCollider::ComponentRegistryEntry->mName = "Collider";
  TacCollider::ComponentRegistryEntry->mNetworkBits = TacColliderBits;
  TacCollider::ComponentRegistryEntry->mCreateFn = TacCreateColliderComponent;
  TacCollider::ComponentRegistryEntry->mDestroyFn = TacDestroyColliderComponent;
  TacCollider::ComponentRegistryEntry->mDebugImguiFn = TacColliderDebugImgui;
}
static void TacSpaceInitPhysics()
{
  TacPhysics::SystemRegistryEntry = TacSystemRegistry::Instance()->RegisterNewEntry();
  TacPhysics::SystemRegistryEntry->mCreateFn = TacCreatePhysicsSystem;
  TacPhysics::SystemRegistryEntry->mName = "Physics";
  TacPhysics::SystemRegistryEntry->mDebugImGui = TacPhysicsDebugImgui;
  TacSpaceInitPhysicsTerrain();
  TacSpaceInitPhysicsCollider();
}

void TacSpaceInit()
{
  TacSpaceInitGraphics();
  TacSpaceInitPhysics();
  for( TacSystemRegistryEntry* entry : TacSystemRegistry::Instance()->mEntries )
  {
    TacAssert( entry->mName.size() );
  }
}
