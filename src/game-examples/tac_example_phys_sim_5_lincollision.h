#pragma once

#include "src/game-examples/tac_examples.h"
#include "src/common/math/tac_vector3.h"
#include "src/common/math/tac_matrix3.h"

namespace Tac
{
  struct Errors;

  struct ExamplePhys5SimObj
  {
    ExamplePhys5SimObj();
    float mMass = 5;
    float mElasticity = 0.5f;
    float mRadius = 1.0f;
    v3 mColor;

    v3 mLinPos{};
    v3 mLinVel{};
    v3 mLinForceAccum{};

    m3 mAngRot = m3::Identity();
    m3 mAngInvInertiaTensor{};
    v3 mAngVel{};
    v3 mAngMomentum{};
    v3 mAngTorqueAccum{};

    // should be called when the radius or mass changes
    void ComputeInertiaTensor();
    void AddForce(v3);
    void BeginFrame();
    void Integrate();
  };


  struct ExamplePhysSim5LinCollision : public Example
  {
    ExamplePhysSim5LinCollision();
    void Update( Errors& ) override;
    void Draw( const ExamplePhys5SimObj&);
    v3 GetKeyboardForce();

    ExamplePhys5SimObj mPlayer;
    ExamplePhys5SimObj mObstacle;
  };
}
