#pragma once
#include "common/math/tacVector3.h"
#include "taccomponent.h"
#include "common/tacPreprocessor.h"

// This is an upright capsule
struct TacCollider : public TacComponent
{
  static TacCollider* GetCollider( TacEntity* );
  TacComponentRegistryEntry* GetEntry() override;
  static TacComponentRegistryEntry* ComponentRegistryEntry;

  //TacComponentRegistryEntryIndex GetComponentType() override { return TacComponentRegistryEntryIndex::Collider; }
  v3 mVelocity = {};
  float mRadius = 0.5f;
  float mTotalHeight = 2.0f;
};

const TacVector< TacNetworkBit > TacColliderBits = [](){
  TacCollider collider;
  auto colliderAddress = ( char* )&collider;
  auto offsetVelocity = int( ( char* )&collider.mVelocity - colliderAddress );
  auto offsetRadius = int( ( char* )&collider.mRadius - colliderAddress );
  auto offsetTotalHeight = int( ( char* )&collider.mTotalHeight - colliderAddress );
  TacVector< TacNetworkBit > result = 
  {
    { "mVelocity", offsetVelocity, sizeof( float ), 3 },
    { "mRadius", offsetRadius, sizeof( float ), 1 },
    { "mTotalHeight", offsetTotalHeight, sizeof( float ), 1 },
  };
  return result;
}();
