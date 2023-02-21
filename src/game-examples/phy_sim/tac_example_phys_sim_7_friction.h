#pragma once

#include "src/game-examples/tac_examples.h"
#include "src/common/math/tac_vector3.h"
#include "src/common/tac_common.h"

namespace Tac
{
  struct ExamplePhysSim7Friction : public Example
  {
    ExamplePhysSim7Friction();
    ~ExamplePhysSim7Friction() override;
    void Update( Errors& ) override;
  };
}
