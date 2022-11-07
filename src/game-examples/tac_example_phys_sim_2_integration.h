#pragma once

#include "src/game-examples/tac_examples.h"
#include "src/common/math/tac_vector3.h"
#include "src/common/containers/tac_ring_array.h"

namespace Tac
{
  struct Errors;

  enum IntegrationMode
  {
    Euler, // Euler's method
    SemiImplicitEuler, // aka 'Symplectic', or 'Symplectic Euler'
    RK4, // Runge-Kutta 4
    Count
  };

  struct ExamplePhysSim2Integration : public Example
  {
    ExamplePhysSim2Integration();
    ~ExamplePhysSim2Integration() override;
    void Update( Errors& ) override;
    void UI();
    void TrackPositions();
    void DrawPositions();
    void Reset();
    v3   GetForce();
    v3   GetAcceleration();
    v3   GetCentripetalAcceleration();
    v3   GetCentripetalForce();

    static const int            kRingCount = 25;
    IntegrationMode             mIntegrationMode{};
    v3                          mPosition;
    v3                          mVelocity;
    float                       mMass = 10.0f;
    RingArray< v3, kRingCount > mPositions;
    float                       timer = 0;
    float                       mBallRadius = 0.25f;
    float                       mOrbitRadius = 1;
    float                       mDuration;
    float                       mAngularVelocity;
  };
}
