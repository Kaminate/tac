
#pragma once
#include "src/common/math/tacVector3.h"
#include "src/common/tacPreprocessor.h"
#include "src/space/tacComponent.h"

namespace Tac
{
// This is an upright capsule
struct Collider : public Component
{
  static void SpaceInitPhysicsCollider();
  static Collider* GetCollider( Entity* );
  ComponentRegistryEntry* GetEntry() const override;
  static ComponentRegistryEntry* ColliderComponentRegistryEntry;

  //ComponentRegistryEntryIndex GetComponentType() override { return ComponentRegistryEntryIndex::Collider; }
  v3 mVelocity = {};
  float mRadius = 0.5f;
  float mTotalHeight = 2.0f;
};

const Vector< NetworkBit > ColliderBits = [](){
  Collider collider;
  auto colliderAddress = ( char* )&collider;
  auto offsetVelocity = int( ( char* )&collider.mVelocity - colliderAddress );
  auto offsetRadius = int( ( char* )&collider.mRadius - colliderAddress );
  auto offsetTotalHeight = int( ( char* )&collider.mTotalHeight - colliderAddress );
  Vector< NetworkBit > result = 
  {
    { "mVelocity", offsetVelocity, sizeof( float ), 3 },
    { "mRadius", offsetRadius, sizeof( float ), 1 },
    { "mTotalHeight", offsetTotalHeight, sizeof( float ), 1 },
  };
  return result;
}();


}

