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
    void Update( Errors& ) override;
    void UI();
    void TrackPositions();
    void DrawPositions();
    void Reset();

    static auto GetForce(v3 pos) -> v3;
    static auto GetAcceleration(v3 pos) -> v3;
    static auto GetCentripetalAcceleration(v3 pos) -> v3;
    static auto GetCentripetalForce(v3 pos) -> v3;

    static constexpr int kRingCount { 25 };
    using Positions = RingArray< v3, kRingCount >;

    IntegrationMode mIntegrationMode{};
    v3              mPosition;
    v3              mVelocity;
    Positions       mPositions;
    float           timer {};
    static float    mBallRadius;
    static float    mOrbitRadius;
    static float    mDuration;
    static float    mMass;
    static float    mAngularVelocity;
  };
}
