#pragma once

#include "tac-examples/tac_examples.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
  struct ExampleMeta : public Example
  {
    void Update( Errors& ) override;
    bool mShouldRunTests { true };
  };
} // namespace Tac
