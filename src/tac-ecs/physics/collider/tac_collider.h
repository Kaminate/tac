#pragma once

#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-ecs/component/tac_component.h"

namespace Tac
{
  // This is an upright capsule
  struct Collider : public Component
  {
    static Collider*               CreateCollider( World* );
    static Collider*               GetCollider( Entity* );
    const ComponentRegistryEntry*  GetEntry() const override;
    v3                             mVelocity = {};
    float                          mRadius = 0.5f;
    float                          mTotalHeight = 2.0f;
  };
  void RegisterColliderComponent();
}

