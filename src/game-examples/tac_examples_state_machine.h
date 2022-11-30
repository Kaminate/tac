#include "src/game-examples/tac_examples.h"

namespace Tac
{
  struct Errors;
  Example* GetCurrExample();
  void SetNextExample(int);
  void ExampleStateMachineUnint();
  int GetCurrExampleIndex();
  void ExampleStateMachineUpdate(Errors&);
} // namespace Tac
