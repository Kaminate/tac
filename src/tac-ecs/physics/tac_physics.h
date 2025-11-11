#pragma once

#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/containers/tac_set.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

#include "tac-ecs/system/tac_system.h"
#include "tac-ecs/tac_space.h"

namespace Tac
{
  struct Physics : public System
  {
    Physics();

    auto CreateCollider() -> Collider*;
    void DestroyCollider( Collider* );

    auto CreateTerrain() -> Terrain*;
    void DestroyTerrain( Terrain* );

    void Integrate();
    void Narrowphase();
    void DebugDrawCapsules();
    void DebugDrawTerrains();

    // ---------------------------------------------------------------------------------------------

    void Update() override;
    void DebugImgui() override;

    // ---------------------------------------------------------------------------------------------

    static void SpaceInitPhysics();
    static auto GetSystem( dynmc World* ) -> dynmc Physics*;
    static auto GetSystem( const World* ) -> const Physics*;
    static SystemInfo*          sInfo;

    // ---------------------------------------------------------------------------------------------

    using Terrains = Set< Terrain* >;
    using Colliders = Set< Collider* >;

    // ---------------------------------------------------------------------------------------------

    Colliders mColliders                {};
    Terrains  mTerrains                 {};
    bool      mDebugDrawCollision       { true };
    float     mGravity                  {};
    bool      mGravityOn                {};
    v3        mDebugDrawCapsuleColor    {};
    bool      mShouldDebugDrawCapsules  {};
    bool      mRunning                  {};
    v3        mDebugDrawTerrainColor    {};
    bool      mShouldDebugDrawTerrains  {};
    bool      mShouldIntegrate          {};
    bool      mShouldNarrowphase        {};
    bool      mGJKDebugging             { true };
    int       mGJKDebugMaxIter          { 10 };
    int       mGJKDebugMaxEPAIter       {};
  };

} // namespace Tac

