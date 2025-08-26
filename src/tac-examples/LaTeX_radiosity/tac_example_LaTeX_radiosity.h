#pragma once

#include "tac-examples/tac_examples.h"

namespace Tac
{
  struct ExampleLaTeXRadiosity : public Example
  {
    void LazyInit( Errors& );
    void Update( Errors& ) override;
  };
}
