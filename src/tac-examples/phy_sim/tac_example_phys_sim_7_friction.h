#pragma once

#include "tac-examples/tac_examples.h"
#include "tac-std-lib/math/tac_vector3.h"

namespace Tac
{
  struct ExamplePhysSim7Friction : public Example
  {
    ExamplePhysSim7Friction();
    ~ExamplePhysSim7Friction() override;
    void Update( UpdateParams, Errors& ) override;
  };
}
