#pragma once
#include "common/math/tacVector3.h"
#include "tacspacetypes.h"
#include "tacsystem.h"
#include "tacgjk.h"
#include <set>
#include <list>

struct TacCollider;
struct TacTerrainOBB
{
  v3 mPos;
  v3 mHalfExtents;
  v3 mEulerRads;
};

struct TacTerrain
{
  TacVector< TacTerrainOBB > mTerrainOBBs;

  // heightmap
};

struct TacHeightmap
{
  // does it make sense to use the same heightmap for rendering and physics?
  // what about tesellation close to camera?
  // what about streaming?
};

struct TacCollideResult
{
  bool mCollided = false;

};
TacCollideResult TacCollide( const TacHeightmap* heightmap, const TacCollider* collider );

struct TacPhysics : public TacSystem
{
  TacPhysics();
  //const TacVector< TacComponentRegistryEntryIndex >& GetManagedComponentTypes() override;
  //TacComponent* CreateComponent( TacComponentRegistryEntryIndex componentType ) override;
  //void DestroyComponent( TacComponent* component ) override;
  //TacSystemType GetSystemType() override { return TacSystemType::Physics; }

  TacCollider* CreateCollider();
  TacTerrain* CreateTerrain();

  static TacSystemRegistryEntry* SystemRegistryEntry;
  static TacPhysics* GetSystem( TacWorld* );

  void Update() override;
  void Integrate();
  void Narrowphase();
  void DebugImgui() override;
  void DebugDrawCapsules();
  void DebugDrawTerrains();

  std::set< TacCollider* > mColliders;
  std::set< TacTerrain* > mTerrains;

  bool mDebugDrawCollision = true;
  float mGravity;
  bool mGravityOn;
  v3 mDebugDrawCapsuleColor;
  bool mShouldDebugDrawCapsules;
  bool mRunning;
  v3 mDebugDrawTerrainColor;
  bool mShouldDebugDrawTerrains;
  bool mShouldIntegrate;
  bool mShouldNarrowphase;
  bool mGJKDebugging = true;
  int mGJKDebugMaxIter = 10;
  int mGJKDebugMaxEPAIter = 0;
};
