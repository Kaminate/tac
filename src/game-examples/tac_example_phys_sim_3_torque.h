#pragma once

#include "src/game-examples/tac_examples.h"
#include "src/common/math/tac_vector3.h"

namespace Tac
{
  struct Errors;
  struct ExamplePhysSim3Torque : public Example
  {
    ExamplePhysSim3Torque();
    ~ExamplePhysSim3Torque() override;
    void Update( Errors& ) override;
  };
}
