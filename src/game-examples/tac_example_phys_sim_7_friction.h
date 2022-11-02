#pragma once

#include "src/game-examples/tac_examples.h"
#include "src/common/math/tac_vector3.h"

namespace Tac
{
  struct Errors;
  struct ExamplePhysSim7Friction : public Example
  {
    ExamplePhysSim7Friction();
    ~ExamplePhysSim7Friction() override;
    void Update( Errors& ) override;
    const char* GetName() const override;
  };
}
