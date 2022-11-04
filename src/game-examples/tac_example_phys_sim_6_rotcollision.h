#pragma once

#include "src/game-examples/tac_examples.h"
#include "src/common/math/tac_vector3.h"

namespace Tac
{
  struct Errors;
  struct ExamplePhysSim6RotCollision : public Example
  {
    ExamplePhysSim6RotCollision();
    ~ExamplePhysSim6RotCollision() override;
    void Update( Errors& ) override;
  };
}
