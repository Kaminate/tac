#pragma once

#include "src/game-examples/tac_examples.h"
#include "src/common/math/tac_vector3.h"
#include "src/common/math/tac_matrix3.h"

namespace Tac
{
  struct Errors;
  struct ExamplePhysSim3Torque : public Example
  {
    ExamplePhysSim3Torque();
    ~ExamplePhysSim3Torque() override;
    void Update( Errors& ) override;
    void Integrate();
    v3 GetKeyboardForce();
    v3 GetDragForce();
    v3 GetDragTorque();

    float mMass = 10;
    float mWidth = 2;

    m3 mOrientation = m3::Identity();
    v3 mPosition = {};
    
    // linear dynamics
    v3 mVelocity {};
    v3 mAngularVelocity{};
    
    // rotational dynamics
    m3 mInverseMoments{}; // inverse of inertia tensor
    v3 mAngularMomentum {};

    v3 mForceAccumWs ={0,0,0};
    v3 mTorqueAccumWs ={};
  };
}
