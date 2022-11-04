#pragma once

#include "src/game-examples/tac_examples.h"

namespace Tac
{
  struct Errors;
  struct ExampleMeta : public Example
  {
    void Update( Errors& ) override;
    bool mShouldRunTests = true;
  };

}
