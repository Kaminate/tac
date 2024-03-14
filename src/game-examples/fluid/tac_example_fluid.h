#pragma once

#include "src/game-examples/tac_examples.h"

namespace Tac { struct Errors; }
namespace Tac
{

  struct ExampleFluid : public Example
  {
    void Update( Errors& ) override;
  };

}
