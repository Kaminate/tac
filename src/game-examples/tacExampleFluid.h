#pragma once

#include "src/game-examples/tacExamples.h"

namespace Tac
{

  struct Errors;
  struct ExampleFluid : public Example
  {
    void Init( Errors& ) override;
    void Update( Errors& ) override;
    void Uninit( Errors& ) override;
    const char* GetName() const override { return "Fluid"; }
  };

}
