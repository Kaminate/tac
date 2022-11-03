#pragma once

#include "src/game-examples/tac_examples.h"
#include "src/common/math/tac_vector3.h"

namespace Tac
{
  struct Errors;

  enum IntegrationMode
  {
    Euler,
    Count
  };

  struct ExamplePhysSim2Integration : public Example
  {
    ExamplePhysSim2Integration();
    ~ExamplePhysSim2Integration() override;
    void Update( Errors& ) override;
    const char* GetName() const override;
    void UI();
    void TrackPositions();
    void DrawPositions();
    void Reset();

    IntegrationMode mIntegrationMode{};

    v3 mPosition;
    v3 mVelocity;
    float mMass;

    const int poscapacity = 1000;
    v3* positions;
    int poscount = 0;
    int iposition = 0;


    float timer = 0;
    float mBallRadius;
    float mOrbitRadius;
  };
}
