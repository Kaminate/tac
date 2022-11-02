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
  }

  struct ExamplePhysSim2Integration : public Example
  {
    ExamplePhysSim2Integration();
    ~ExamplePhysSim2Integration() override;
    void Update( Errors& ) override;
    const char* GetName() const override;

    IntegrationMode mIntegrationMode{};

    v3 mPosition;
    v3 mVelocity;
    float mMass;
  };
}
