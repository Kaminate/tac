
#pragma once
#include "src/common/math/tac_vector3.h"
#include "src/common/tac_error_handling.h"
#include "src/space/tac_system.h"
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

    Collider* CreateCollider();
    void      DestroyCollider( Collider* collider );

    Terrain*  CreateTerrain();
    void      DestroyTerrain( Terrain* terrain );

    static void                 SpaceInitPhysics();
    static SystemRegistryEntry* PhysicsSystemRegistryEntry;
    static Physics*             GetSystem( World* );

    void Update() override;
    void Integrate();
    void Narrowphase();
    void DebugImgui() override;
    void DebugDrawCapsules();
    void DebugDrawTerrains();

    std::set< Collider* > mColliders;
    std::set< Terrain* >  mTerrains;


    bool  mDebugDrawCollision = true;
    float mGravity;
    bool  mGravityOn;
    v3    mDebugDrawCapsuleColor;
    bool  mShouldDebugDrawCapsules;
    bool  mRunning;
    v3    mDebugDrawTerrainColor;
    bool  mShouldDebugDrawTerrains;
    bool  mShouldIntegrate;
    bool  mShouldNarrowphase;
    bool  mGJKDebugging = true;
    int   mGJKDebugMaxIter = 10;
    int   mGJKDebugMaxEPAIter = 0;
  };

}

