#pragma once

#include "src/game-examples/tac_examples.h"
#include "src/common/math/tac_vector3.h"

namespace Tac
{
  struct Errors;
  struct ExamplePhysSim5LinCollision : public Example
  {
    ExamplePhysSim5LinCollision();
    ~ExamplePhysSim5LinCollision() override;
    void Update( Errors& ) override;
    const char* GetName() const override;
  };
}
