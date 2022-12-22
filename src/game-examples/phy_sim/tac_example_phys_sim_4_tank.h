#pragma once

#include "src/game-examples/tac_examples.h"
#include "src/common/math/tac_vector3.h"

namespace Tac
{
  struct Errors;
  struct ExamplePhysSim4Tank : public Example
  {
    ExamplePhysSim4Tank();
    ~ExamplePhysSim4Tank() override;
    void Update( Errors& ) override;
  };
}
