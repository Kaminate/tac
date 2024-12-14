#pragma once

#include "tac-examples/tac_examples.h"
#include "tac-std-lib/math/tac_vector3.h"

namespace Tac
{
  struct ExamplePhysSim4Tank : public Example
  {
    ExamplePhysSim4Tank();
    ~ExamplePhysSim4Tank() override;
    void Update( Errors& ) override;
  };
}
