#pragma once

#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/material/tac_material.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h" // Mesh

namespace Tac
{
  struct PreBakeScene
  {
    struct PatchPower
    {
      using Vtxs = Array< v3, 3 >;

      float GetUnshotPower() const;
      v3    GetRandomSurfacePoint() const;

      Vtxs  mTriVerts             {}; // worldspace
      v3    mTotalPower           {};
      v3    mCurrentReceivedPower {};
      v3    mCurrentUnshotPower   {};
      v3    mUnitNormal           {};
      float mArea                 {};
    };

    struct Instance
    {
      using PatchPowers = Vector< PatchPower >;

      const Entity*   mEntity      {};
      const Model*    mModel       {};
      const Mesh*     mMesh        {};
      const Material* mMaterial    {};
      PatchPowers     mPatchPowers {};
    };

    struct RaycastResult
    {
      PatchPower*     mHitPatch         {};
      const Material* mHitPatchMaterial {};
      float           mT                {};
    };

    auto Raycast( PatchPower* fromPatch, RayTriangle::Ray ) -> RaycastResult;
    auto GetMesh( const Model* ) -> const Mesh*;
    void Init( const World* );
    auto ComputeTotalUnshotPower() -> float;
    void Bake();

    v3 mDebugSrcPos{};
    PatchPower* mDebugSrcPatch{};
    v3 mDebugDstPos{};
    PatchPower* mDebugDstPatch{};
    bool mDebugLine{};

    Vector< Instance > mInstances;
  };
} // namespace Tac


