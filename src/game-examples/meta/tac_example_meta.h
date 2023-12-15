#pragma once

#include "src/game-examples/tac_examples.h"
#include "src/common/tac_core.h"

namespace Tac
{
  struct ExampleMeta : public Example
  {
    void Update( Errors& ) override;
    bool mShouldRunTests = true;
  };

}
