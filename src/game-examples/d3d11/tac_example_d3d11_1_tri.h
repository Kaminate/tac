#pragma once

#include "src/game-examples/tac_examples.h"
#include "src/common/tac_common.h"

namespace Tac
{
  struct ExampleMeta : public Example
  {
    void Init( Errors& ) override;
    void Update( Errors& ) override;
    void Uninit( Errors& ) override;
    const char* GetName() const override;
  };

}
