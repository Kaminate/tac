#pragma once

#include "src/game-examples/tac_examples.h"

namespace Tac
{
  struct ExampleD3D11Tri : public Example
  {
    void Init( Errors& ) override;
    void Update( Errors& ) override;
    void Uninit( Errors& ) override;
    const char* GetName() const override;
  };
}
