#pragma once

#include "tac-examples/tac_examples.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_matrix3.h"

namespace Tac
{
  struct ExamplePhysSim3Torque : public Example
  {
    ExamplePhysSim3Torque();
    void Update( Errors& ) override;
    void Integrate();
    auto GetDragForce() -> v3;
    auto GetDragTorque() -> v3;

    float mMass       { 10 };
    float mWidth      { 2 };

    m3 mRot           { m3::Identity() };
    v3 mPos           {};
    
    // linear dynamics
    v3 mLinVel        {};
    v3 mAngVel        {};
    
    // rotational dynamics
    m3 mInvMoment     {}; // inverse of inertia tensor
    v3 mAngMomentum   {};

    // frame accumulators
    v3 mForceAccumWs  {};
    v3 mTorqueAccumWs {};
  };
}
