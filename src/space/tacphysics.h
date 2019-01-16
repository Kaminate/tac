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
};

struct TacPhysics : public TacSystem
{
  TacPhysics();
  TacComponent* CreateComponent( TacComponentType componentType ) override;
  void DestroyComponent( TacComponent* component ) override;
  TacSystemType GetSystemType() override { return TacSystemType::Physics; }

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
