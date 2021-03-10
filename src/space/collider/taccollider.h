#pragma once

#include "src/common/math/tacVector3.h"
#include "src/common/tacPreprocessor.h"
#include "src/space/tacComponent.h"

namespace Tac
{
  // This is an upright capsule
  struct Collider : public Component
  {
    static Collider*               GetCollider( Entity* );
    ComponentRegistryEntry*        GetEntry() const override;
    static ComponentRegistryEntry* ColliderComponentRegistryEntry;
    v3                             mVelocity = {};
    float                          mRadius = 0.5f;
    float                          mTotalHeight = 2.0f;
  };
  void RegisterColliderComponent();
}

