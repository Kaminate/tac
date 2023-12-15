#pragma once

#include "src/game-examples/tac_examples.h"
#include "src/common/tac_core.h"

namespace Tac
{
  Example* GetCurrExample();
  void SetNextExample(int);
  void ExampleStateMachineUnint();
  int GetCurrExampleIndex();
  void ExampleStateMachineUpdate(Errors&);
} // namespace Tac
