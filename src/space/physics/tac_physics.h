#pragma once

#include "src/common/math/tac_vector3.h"
#include "src/space/tac_system.h"
#include "src/space/tac_space.h"

import std;
//#include <set>

namespace Tac
{
  struct Physics : public System
  {
    Physics();

    Collider* CreateCollider();
    void      DestroyCollider( Collider* );

    Terrain*  CreateTerrain();
    void      DestroyTerrain( Terrain* );

    void      Integrate();
    void      Narrowphase();
    void      DebugDrawCapsules();
    void      DebugDrawTerrains();

    // ---------------------------------------------------------------------------------------------

    void      Update() override;
    void      DebugImgui() override;

    // ---------------------------------------------------------------------------------------------

    static void                 SpaceInitPhysics();
    static SystemRegistryEntry* PhysicsSystemRegistryEntry;
    static Physics*             GetSystem( World* );

    // ---------------------------------------------------------------------------------------------

    using Terrains = std::set< Terrain* >;
    using Colliders = std::set< Collider* >;

    // ---------------------------------------------------------------------------------------------

    Colliders mColliders;
    Terrains  mTerrains;
    bool      mDebugDrawCollision = true;
    float     mGravity;
    bool      mGravityOn;
    v3        mDebugDrawCapsuleColor;
    bool      mShouldDebugDrawCapsules;
    bool      mRunning;
    v3        mDebugDrawTerrainColor;
    bool      mShouldDebugDrawTerrains;
    bool      mShouldIntegrate;
    bool      mShouldNarrowphase;
    bool      mGJKDebugging = true;
    int       mGJKDebugMaxIter = 10;
    int       mGJKDebugMaxEPAIter = 0;
  };

} // namespace Tac

