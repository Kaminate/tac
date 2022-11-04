#pragma once

#include "src/game-examples/tac_examples.h"

namespace Tac
{

  struct Errors;
  struct ExampleFluid : public Example
  {
    void Update( Errors& ) override;
  };

}
