#pragma once

#include "src/game-examples/tac_examples.h"

namespace Tac
{
  struct Errors;
  struct ExampleMeta : public Example
  {
    void Init( Errors& ) override;
    void Update( Errors& ) override;
    void Uninit( Errors& ) override;
    const char* GetName() const override;
  };

}
