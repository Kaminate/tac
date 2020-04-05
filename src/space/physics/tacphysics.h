
#pragma once
#include "src/common/math/tacVector3.h"
#include "src/common/tacErrorHandling.h"
#include "src/space/tacSystem.h"
#include <set>
#include <list>

namespace Tac
{
struct Collider;
struct Terrain;
struct Debug3DDrawData;


struct Physics : public System
{
  Physics();
  //const Vector< ComponentRegistryEntryIndex >& GetManagedComponentTypes() override;
  //Component* CreateComponent( ComponentRegistryEntryIndex componentType ) override;
  //void DestroyComponent( Component* component ) override;
  //SystemType GetSystemType() override { return SystemType::Physics; }

  Collider* CreateCollider();
  void DestroyCollider( Collider* collider );

  Terrain* CreateTerrain();
  void DestroyTerrain( Terrain* terrain );

  static void SpaceInitPhysics();
  static SystemRegistryEntry* PhysicsSystemRegistryEntry;
  static Physics* GetSystem( World* );

  void Update() override;
  void Integrate();
  void Narrowphase();
  void DebugImgui() override;
  void DebugDrawCapsules();
  void DebugDrawTerrains();

  std::set< Collider* > mColliders;
  std::set< Terrain* > mTerrains;


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

}

