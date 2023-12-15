#pragma once

#include "src/game-examples/tac_examples.h"
#include "src/common/math/tac_vector3.h"
#include "src/common/tac_core.h"

namespace Tac
{
  struct ExamplePhysSim4Tank : public Example
  {
    ExamplePhysSim4Tank();
    ~ExamplePhysSim4Tank() override;
    void Update( Errors& ) override;
  };
}
