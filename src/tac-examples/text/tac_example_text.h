#pragma once

#include "tac-examples/tac_examples.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
  struct ExampleText : public Example
  {
    void Update( Errors& ) override;
  };


}
