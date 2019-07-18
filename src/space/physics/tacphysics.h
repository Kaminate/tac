#pragma once
#include "common/math/tacVector3.h"
#include "common/tacErrorHandling.h"
#include "tacspacetypes.h"
#include "tacsystem.h"
#include "tacgjk.h"
#include <set>
#include <list>

struct TacCollider;
struct TacTerrain;
struct TacDebug3DDrawData;


struct TacPhysics : public TacSystem
{
  TacPhysics();
  //const TacVector< TacComponentRegistryEntryIndex >& GetManagedComponentTypes() override;
  //TacComponent* CreateComponent( TacComponentRegistryEntryIndex componentType ) override;
  //void DestroyComponent( TacComponent* component ) override;
  //TacSystemType GetSystemType() override { return TacSystemType::Physics; }

  TacCollider* CreateCollider();
  void DestroyCollider( TacCollider* collider );

  TacTerrain* CreateTerrain();
  void DestroyTerrain( TacTerrain* terrain );

  static TacSystemRegistryEntry* SystemRegistryEntry;
  static TacPhysics* GetSystem( TacWorld* );

  void Update() override;
  void Integrate();
  void Narrowphase();
  void DebugImgui() override;
  void DebugDrawCapsules();
  void DebugDrawTerrains();
  void LoadTestHeightmap();

  std::set< TacCollider* > mColliders;
  std::set< TacTerrain* > mTerrains;

  //TacTexture* mTestHeightmap = nullptr;
  int mTestHeightmapWidth;
  int mTestHeightmapHeight;
  TacVector< uint8_t > mTestHeightmapImageMemory;
  TacErrors mTestHeightmapLoadErrors;

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