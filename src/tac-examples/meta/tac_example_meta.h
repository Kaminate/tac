#pragma once

#include "tac-examples/tac_examples.h"

namespace Tac { struct Errors; }
namespace Tac
{
  struct ExampleMeta : public Example
  {
    void Update( UpdateParams, Errors& ) override;
    bool mShouldRunTests { true };
  };

}
