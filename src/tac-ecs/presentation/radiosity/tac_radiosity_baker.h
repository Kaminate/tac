#pragma once

#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/mutex/tac_mutex.h"
#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/material/tac_material.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h" // Mesh
#include "tac-engine-core/job/tac_job_queue.h"

namespace Tac
{
  struct PreBakeScene : public Job
  {
    struct PatchPower
    {
      using VtxPos = Array< v3, 3 >;
      using VtxNor = Array< v3, 3 >;

      float GetUnshotPower() const;
      v3    GetUniformRandomSurfacePoint() const;

      VtxPos  mTriVerts             {}; // worldspace
      VtxNor  mTriNormals           {}; // worldspace
      v3      mTotalPower           {};
      v3      mCurrentReceivedPower {}; // per-iteration
      v3      mCurrentUnshotPower   {}; // per-iteration
      v3      mUnitNormal           {};
      float   mArea                 {};
    };

    using PatchPowers = Vector< PatchPower >;

    struct Instance
    {

      int VertexCount() const { return mPatchPowers.size() * 3; }

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
    void Execute( Errors& )override;
    void SaveToFile( Errors& );

    using Instances = Vector< Instance >;

    v3          mDebugSrcPos   {};
    PatchPower* mDebugSrcPatch {};
    v3          mDebugDstPos   {};
    PatchPower* mDebugDstPatch {};
    bool        mDebugLine     {};
    Instances   mInstances     {};
    World*      mWorld         {};
  };
} // namespace Tac


