#pragma once

#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-ecs/component/tac_component.h"

namespace Tac
{
  struct Collider : public Component
  {
    enum Type
    {
      kSphere,
      kCapsule,
      kRectanglarPrism,
    };
    struct SphereData
    {
      float mRadius;
    };
    struct CapsuleData
    {
      float mRadius     ;//{ 0.5f };
      float mTotalHeight;//{ 2.0f };
    };
    struct RectangularPrismData
    {
      float mHalfX;
      float mHalfY;
      float mHalfZ;
    };

    static void RegisterComponent();
    static auto CreateCollider( World* ) -> Collider*;
    static auto GetCollider( Entity* ) -> Collider*;
    auto GetEntry() const -> const ComponentInfo* override;

    union
    {
      SphereData           mSphereData;
      CapsuleData          mCapsuleData;
      RectangularPrismData mRectangularPrismData;
    };
    Type mType     {};
    v3   mVelocity {};
  };
  TAC_META_DECL( Collider );
}

