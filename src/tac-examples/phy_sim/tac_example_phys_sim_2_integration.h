#pragma once

#include "tac-examples/tac_examples.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/containers/tac_ring_array.h"

namespace Tac
{
  enum class IntegrationMode
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
    void Update( UpdateParams, Errors& ) override;
    void UI();
    void TrackPositions();
    void DrawPositions();
    void Reset();

    static v3   GetForce(v3 pos);
    static v3   GetAcceleration(v3 pos);
    static v3   GetCentripetalAcceleration(v3 pos);
    static v3   GetCentripetalForce(v3 pos);

    static const int            kRingCount { 25 };
    IntegrationMode             mIntegrationMode{};
    v3                          mPosition;
    v3                          mVelocity;
    RingArray< v3, kRingCount > mPositions;
    float                       timer {};
    static float                mBallRadius;
    static float                mOrbitRadius;
    static float                mDuration;
    static float                mMass;
    static float                mAngularVelocity;
  };
}
